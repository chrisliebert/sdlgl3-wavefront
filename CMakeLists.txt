CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(sdlglapp)

ADD_EXECUTABLE(sdlglapp
	dependencies/tinyobjloader/tiny_obj_loader.h
	dependencies/tinyobjloader/tiny_obj_loader.cc
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

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

include_directories(include)
include_directories(dependencies/glm)
include_directories(dependencies/tinyobjloader)
include_directories(${OpenGL_INCLUDE_DIRS})
include_directories(${GLEW_INCLUDE_DIRS})
link_directories(${OpenGL_LIBRARY_DIRS})
link_directories(${GLEW_LIBRARY_DIRS})
add_definitions(${OpenGL_DEFINITIONS})

if(NOT OPENGL_FOUND)
    message(ERROR " OPENGL not found!")
endif(NOT OPENGL_FOUND)

INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${SDL2_image_INCLUDE_DIRS})
LINK_DIRECTORIES(${SDL2_LIBRARY_DIRS})
LINK_DIRECTORIES(${SDL2_image_Library_DIRS})
TARGET_LINK_LIBRARIES(sdlglapp
	${SDL2_LIBRARIES} ${OPENGL_LIBRARIES} ${GLEW_LIBRARY} ${SDL2_image_LIBRARIES}
)
