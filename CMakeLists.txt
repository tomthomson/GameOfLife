cmake_minimum_required(VERSION 2.6)
project(GameOfLife)

###
# set environment variables
###
# for finding local cmake scripts
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
# for finding GLEW libs; also add ${GLEW_LIBRARY} to target_link_libraries
#set(CMAKE_PREFIX_PATH "/home/thomson/apps/NVIDIA_GPU_Computing_SDK/shared/lib/linux")

###
# find additional packages and include corresponding directories
###
# OpenCL
find_package(OpenCL REQUIRED)
include_directories(${OPENCL_INCLUDE_DIR})
# GLUT
find_package(GLUT REQUIRED)
include_directories(${GLUT_INCLUDE_DIR})
# OpenGL
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})
# GLEW
#find_package(GLEW REQUIRED)
#include_directories(${GLEW_INCLUDE_DIR})

###
# include source directories
###
include_directories(src)
include_directories(inc)

###
# build
###
add_executable(GameOfLife src/main.cpp src/GameOfLife.cpp src/anyoption.cpp src/PatternFile.cpp src/KernelFile.cpp)
target_link_libraries(GameOfLife ${OPENCL_LIBRARIES} ${GLUT_LIBRARY} ${OPENGL_LIBRARY})

###
# copy OpenCL kernel file to build directory
###
configure_file(src/kernels.cl . COPYONLY)