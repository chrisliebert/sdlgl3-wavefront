# sdlgl3-wavefront

sdlgl3-wavefront is a starting point template for shader-based OpenGL 3.3 applications which can load multiple wavefront/.obj files and textures. The renderer will load a list of objects into a single vertex buffer and glDrawRangeElementsBaseVertex is used for rendering. Other versions of OpenGL could also be used as long as shaders, buffer objects and glDrawRangeElementsBaseVertex are available.

Frustum extraction and testing algorithms from http://www.crownandcutlass.com/features/technicaldetails/frustum.html have been incorporated into Frustum.cpp. After 3D model/models are loaded each disconnected mesh is put into a SceneNode structure and a bounding radius is calculated. The Renderer.renderer() is passed a camera object so the frustum matrix can be extracted every frame and every scene node in a renderer instance will be tested against the camera's frustum. The models used in this demo were derived from the Google 3D warehouse models and ariel imagery/elevation data was downloaded from http://oregonexplorer.info/

#Build instructions:
Install SDL2, SDL2_image GLEW and CMake.

Generate Makefile/IDE project for example:

cmake -G"Unix Makefiles"

generates Makefile for Linux.
Instructions for Windows can be found [here](doc/Windows_Dev_Setup.html)

#Screenshots:
![alt tag](https://raw.githubusercontent.com/chrisliebert/sdlgl3-wavefront/master/sdlgl3-wavefront_screenshot1.jpg)
![alt tag](https://raw.githubusercontent.com/chrisliebert/sdlgl3-wavefront/master/sdlgl3-wavefront_screenshot2.jpg)
![alt tag](https://raw.githubusercontent.com/chrisliebert/sdlgl3-wavefront/master/sdlgl3-wavefront_screenshot3.jpg)
![alt tag](https://raw.githubusercontent.com/chrisliebert/sdlgl3-wavefront/master/sdlgl3-wavefront_screenshot4.jpg)
![alt tag](https://raw.githubusercontent.com/chrisliebert/sdlgl3-wavefront/master/sdlgl3-wavefront_screenshot5.jpg)
![alt tag](https://raw.githubusercontent.com/chrisliebert/sdlgl3-wavefront/master/sdlgl3-wavefront_screenshot6.jpg)

#License:
sdlgl3-wavefront Licensed under 2 clause BSD.

GLM library licensed under MIT

SDL2 and SDL2_image library licensed under zlib

GLEW licensed under modified BSD and MIT licenses
