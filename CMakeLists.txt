cmake_minimum_required(VERSION 2.6)
project(GameOfLife)

###
# set environment variables
###
# for finding local cmake scripts
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

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

###
# setting default target for building
###
IF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Release)
ENDIF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
message("Building for CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

###
# compiler flags
###
SET(CMAKE_CXX_FLAGS "-g -Wall")

###
# include source directories
###
include_directories(src)
include_directories(inc)

###
# build
###
add_executable(GameOfLife src/main.cpp src/GameOfLife.cpp src/PatternFile.cpp src/KernelFile.cpp)
target_link_libraries(GameOfLife ${OPENCL_LIBRARIES} ${GLUT_LIBRARY} ${OPENGL_LIBRARY})

###
# copy OpenCL kernel file to build directory
###
configure_file(src/kernels.cl . COPYONLY)