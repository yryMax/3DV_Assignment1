#include "gradient_volume.h"
#include <algorithm>
#include <exception>
#include <glm/geometric.hpp>
#include <glm/vector_relational.hpp>
#include <gsl/span>

namespace volume {

// Compute the maximum magnitude from all gradient voxels
static float computeMaxMagnitude(gsl::span<const GradientVoxel> data)
{
    return std::max_element(
        std::begin(data),
        std::end(data),
        [](const GradientVoxel& lhs, const GradientVoxel& rhs) {
            return lhs.magnitude < rhs.magnitude;
        })
        ->magnitude;
}

// Compute the minimum magnitude from all gradient voxels
static float computeMinMagnitude(gsl::span<const GradientVoxel> data)
{
    return std::min_element(
        std::begin(data),
        std::end(data),
        [](const GradientVoxel& lhs, const GradientVoxel& rhs) {
            return lhs.magnitude < rhs.magnitude;
        })
        ->magnitude;
}

// Compute a gradient volume from a volume
static std::vector<GradientVoxel> computeGradientVolume(const Volume& volume)
{
    const auto dim = volume.dims();

    std::vector<GradientVoxel> out(static_cast<size_t>(dim.x * dim.y * dim.z));
    for (int z = 1; z < dim.z - 1; z++) {
        for (int y = 1; y < dim.y - 1; y++) {
            for (int x = 1; x < dim.x - 1; x++) {
                const float gx = (volume.getVoxel(x + 1, y, z) - volume.getVoxel(x - 1, y, z)) / 2.0f;
                const float gy = (volume.getVoxel(x, y + 1, z) - volume.getVoxel(x, y - 1, z)) / 2.0f;
                const float gz = (volume.getVoxel(x, y, z + 1) - volume.getVoxel(x, y, z - 1)) / 2.0f;

                const glm::vec3 v { gx, gy, gz };
                const size_t index = static_cast<size_t>(x + dim.x * (y + dim.y * z));
                out[index] = GradientVoxel { v, glm::length(v) };
            }
        }
    }
    return out;
}

GradientVolume::GradientVolume(const Volume& volume)
    : m_dim(volume.dims())
    , m_data(computeGradientVolume(volume))
    , m_minMagnitude(computeMinMagnitude(m_data))
    , m_maxMagnitude(computeMaxMagnitude(m_data))
{
}

float GradientVolume::maxMagnitude() const
{
    return m_maxMagnitude;
}

float GradientVolume::minMagnitude() const
{
    return m_minMagnitude;
}

glm::ivec3 GradientVolume::dims() const
{
    return m_dim;
}

// This function returns a gradientVoxel at coord based on the current interpolation mode.
GradientVoxel GradientVolume::getGradientInterpolate(const glm::vec3& coord) const
{
    switch (interpolationMode) {
    case InterpolationMode::NearestNeighbour: {
        return getGradientNearestNeighbor(coord);
    }
    case InterpolationMode::Linear: {
        return getGradientLinearInterpolate(coord);
    }
    case InterpolationMode::Cubic: {
        // No cubic in this case, linear is good enough for the gradient.
        return getGradientLinearInterpolate(coord);
    }
    default: {
        throw std::exception();
    }
    };
}

// This function returns the nearest neighbour given a position in the volume given by coord.
// Notice that in this framework we assume that the distance between neighbouring voxels is 1 in all directions
GradientVoxel GradientVolume::getGradientNearestNeighbor(const glm::vec3& coord) const
{
    if (glm::any(glm::lessThan(coord, glm::vec3(0))) || glm::any(glm::greaterThanEqual(coord, glm::vec3(m_dim))))
        return { glm::vec3(0.0f), 0.0f };

    auto roundToPositiveInt = [](float f) {
        return static_cast<int>(f + 0.5f);
    };

    return getGradient(roundToPositiveInt(coord.x), roundToPositiveInt(coord.y), roundToPositiveInt(coord.z));
}

// ======= TODO : IMPLEMENT ========
// Returns the trilinearly interpolated gradinet at the given coordinate.
// Use the linearInterpolate function that you implemented below.
GradientVoxel GradientVolume::getGradientLinearInterpolate(const glm::vec3& coord) const
{
    // Following the explanation of Trilinear Interpolation from https://en.wikipedia.org/wiki/Trilinear_interpolation
    // First obtain the lattice points needed to calculate xd, yd, zd
    // Point 0 is the one below the coordinate and 1 is the one above (in their respective dimensions)
    int x0 = glm::floor(coord.x);
    int x1 = x0 + 1;
    int y0 = glm::floor(coord.y);
    int y1 = y0 + 1;
    int z0 = glm::floor(coord.z);
    int z1 = z0 + 1;

    // Making sure they are in range (clamp from 0 to dimension in that direction - 1 (indices))
    x0 = glm::clamp(x0, 0, m_dim.x - 1);
    x1 = glm::clamp(x1, 0, m_dim.x - 1);
    y0 = glm::clamp(y0, 0, m_dim.y - 1);
    y1 = glm::clamp(y1, 0, m_dim.y - 1);
    z0 = glm::clamp(z0, 0, m_dim.z - 1);
    z1 = glm::clamp(z1, 0, m_dim.z - 1);

    // Then compute differences of the x,y,z coordinates and the smaller coordinate related; this gives xd, yd and zd
    
    // Make them 0 in case x1 == x0 due to clamping (otherwise division by 0)
    float xd = 0;
    float yd = 0;
    float zd = 0;

    // Otherwise compute based on the formula
    if (x1 != x0) xd = (coord.x - x0) / (x1 - x0);
    if (y1 != y0) yd = (coord.y - y0) / (y1 - y0);
    if (z1 != z0) zd = (coord.z - z0) / (z1 - z0);

    // Since we are working with gradients we need to get the respective gradient for each of the cube corners points
    // the cube corners correspond to the combinations of the coordinates calculated aboce

    // LOWER PLANE
    //  - front side
    GradientVoxel c000 = getGradient(x0, y0, z0);
    GradientVoxel c100 = getGradient(x1, y0, z0);
    
    //  - back side
    GradientVoxel c010 = getGradient(x0, y1, z0);
    GradientVoxel c110 = getGradient(x1, y1, z0);

    // UPPER PLANE
    //  - front side
    GradientVoxel c001 = getGradient(x0, y0, z1);
    GradientVoxel c101 = getGradient(x1, y0, z1);

    //  - back side
    GradientVoxel c011 = getGradient(x0, y1, z1);
    GradientVoxel c111 = getGradient(x1, y1, z1);

    // INTERPOLATING
    // First we interpolate along x
    GradientVoxel c00 = linearInterpolate(c000, c100, xd);
    GradientVoxel c01 = linearInterpolate(c001, c101, xd);
    GradientVoxel c10 = linearInterpolate(c010, c110, xd);
    GradientVoxel c11 = linearInterpolate(c011, c111, xd);

    // Then we interpolate along y
    GradientVoxel c0 = linearInterpolate(c00, c10, yd);
    GradientVoxel c1 = linearInterpolate(c01, c11, yd);

    // And then z
    GradientVoxel c = linearInterpolate(c0, c1, zd);

    return c;
}

// ======= TODO : IMPLEMENT ========
// This function should linearly interpolates the value from g0 to g1 given the factor (t).
// At t=0, linearInterpolate should return g0 and at t=1 it returns g1.
GradientVoxel GradientVolume::linearInterpolate(const GradientVoxel& g0, const GradientVoxel& g1, float factor)
{
    glm::vec3 direction = glm::mix(g0.dir, g1.dir, factor);
    float magnitude = glm::mix(g0.magnitude, g1.magnitude, factor);
    return GradientVoxel {direction, magnitude};
}

// This function returns a gradientVoxel without using interpolation
GradientVoxel GradientVolume::getGradient(int x, int y, int z) const
{
    const size_t i = static_cast<size_t>(x + m_dim.x * (y + m_dim.y * z));
    return m_data[i];
}
}