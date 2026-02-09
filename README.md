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
This application is a copy of VulkanStart, except it starts with a different background color, and alternates between that color and another one every 3 seconds. It uses the Prometheus time functions getTime() and getElapsedSeconds() to get the current time and count the time elapsed since the last color change. Ff the time elapsed is greater than 3 seconds, the color channels for the background are changed.
