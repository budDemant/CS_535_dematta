# CS 535 (Computer Graphics)
***Spring 2026***  
***Author: Anthony DeMatteo***  
***Original Author: Dr. Michael J. Reale***  
***SUNY Polytechnic Institute*** 

## Software Dependencies
You will need to install the following manually:
- C++ compilers
- CMake
- Visual Code
- Vulkan SDK

The rest of the dependencies will be automatically fetched by CMake.

## Applications

### VulkanStart
This application should show a multi-color quad on the screen with a cyan background.

### Assign01
This application is a copy of VulkanStart, except it starts with a different background color, and alternates between that color and another one every 3 seconds. It uses the Prometheus time functions getTime() and getElapsedSeconds() to get the current time and count the time elapsed since the last color change. If the time elapsed is greater than 3 seconds, the color channels for the background are changed.

### Assign02
The createShape function procedurally generates a triangle fan mesh, each triangle consisting of a center vertex, an outer vertex, and an adjacent outer vertex. The outer vertices are placed evenly and given color using trigonometric functions (cos and sin). The number of triangles generated is controlled by the maxSub (maximum subdivision) parameter. Increasing this parameter converges the shape into a closer approximation of a circle. The flipWinding parameter is a boolean that demonstrates back-face culling effects.
