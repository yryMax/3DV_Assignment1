## 3DV Assignment 1

Group 12

#### Iso-surface Raycasting
Isosurface raycasting is a threshold-based rendering technique where parts of the image with values below a specified threshold are "peeled away" or discarded, while contours above the threshold are preserved. It is achieved by continuously sampling along a ray: if a voxel’s strength exceeds the threshold, a designated target color is displayed; otherwise, the corresponding pixel is black.

A binary search approach can be employed to achieve higher precision in isosurface raycasting. This method assumes that the strength values along each sampled segment of the ray increase monotonically and search for a position that is most similar with the threshold.


<div style="display: flex; justify-content: space-between;">
  <img src="./isosurface1.png" alt="viewpoint 2" style="width:46%;">
  <img src="./isosurface2.png" alt="viewpoint 3" style="width:46%;">
</div>
<center>basic Iso-surface Raycasting(left), Iso-surface Raycasting with binary search and Phong shading(right)<center>
