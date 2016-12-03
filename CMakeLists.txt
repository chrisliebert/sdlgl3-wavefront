CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(sdlglapp)

ADD_EXECUTABLE(sdlglapp
    dependencies/tinyobjloader/tiny_obj_loader.h
    dependencies/tinyobjloader/tiny_obj_loader.cc
	dependencies/glad/src/glad.c
	dependencies/glad/include/glad/glad.h
	dependencies/glad/include/KHR/khrplatform.h
	
	include/Camera.h
	include/Common.h
	include/Frustum.h
	include/GpuProgram.h
	include/Material.h
	include/SceneNode.h
	include/Renderer.h
	include/Shader.h
	src/Camera.cpp
	src/Frustum.cpp
	src/GpuProgram.cpp
	src/main.cpp
	src/SceneNode.cpp
	src/Renderer.cpp
	src/Shader.cpp
)

INCLUDE(FindPkgConfig)
PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
PKG_SEARCH_MODULE(SDL2_image REQUIRED SDL2_image)

FIND_PACKAGE(OpenGL REQUIRED)

INCLUDE_DIRECTORIES(include)
INCLUDE_DIRECTORIES(dependencies/glm)
INCLUDE_DIRECTORIES(dependencies/tinyobjloader)
INCLUDE_DIRECTORIES(dependencies/glad/include)
INCLUDE_DIRECTORIES(${OpenGL_INCLUDE_DIRS})
LINK_DIRECTORIES(${OpenGL_LIBRARY_DIRS})
ADD_DEFINITIONS(${OpenGL_DEFINITIONS})

IF(NOT OPENGL_FOUND)
    message(ERROR " OPENGL not found!")
ENDIF(NOT OPENGL_FOUND)

INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${SDL2_image_INCLUDE_DIRS})
LINK_DIRECTORIES(${SDL2_LIBRARY_DIRS})
LINK_DIRECTORIES(${SDL2_image_Library_DIRS})

# GLAD requires dl to be linked on Unix systems
IF(UNIX)
	SET(OPENGL_LIBRARIES "dl ${OPENGL_LIBRARIES}")
ENDIF(UNIX)

TARGET_LINK_LIBRARIES(sdlglapp
	${SDL2_LIBRARIES} ${OPENGL_LIBRARIES} ${SDL2_image_LIBRARIES}
)
