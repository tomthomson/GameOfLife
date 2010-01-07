cmake_minimum_required(VERSION 2.8)
project(GameOfLife)

# for finding local cmake scripts
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(GPU_SDK_DIRECTORY "/home/thomson/apps/NVIDIA_GPU_Computing_SDK/")
set(CUDA_SDK_ROOT_DIR "/home/thomson/apps/NVIDIA_GPU_Computing_SDK/")
#set(CMAKE_PREFIX_PATH "/home/thomson/usr/")

include_directories(${GPU_SDK_DIRECTORY}/shared/inc)
include_directories(${GPU_SDK_DIRECTORY}/OpenCL/common/inc)

find_package(OpenCL REQUIRED)
include_directories(${OPENCL_INCLUDE_DIR})
find_package(GLUT REQUIRED)
include_directories(${GLUT_INCLUDE_DIR})
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})
find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIR})

include_directories(src)
include_directories(inc)

add_executable(GameOfLife src/GameOfLifeMain.cpp src/GameOfLife.cpp src/KernelFile.cpp)
target_link_libraries(GameOfLife ${OPENCL_LIBRARIES} ${GLUT_LIBRARY} ${OPENGL_LIBRARY} ${GLEW_LIBRARY})
