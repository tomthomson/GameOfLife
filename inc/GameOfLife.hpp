#ifndef GAMEOFLIFE_H_
#define GAMEOFLIFE_H_

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>
#define __CL_ENABLE_EXCEPTIONS
#define __NO_STD_VECTOR
#define __NO_STD_STRING
#include <CL/cl.hpp>
#include "../inc/KernelFile.hpp"

class GameOfLife {
private:
	cl_float            population;       /**< starting population density */
	cl_uint                   seed;       /**< Seed value for random number generation */
	cl_float                 scale;       /**< paramters of mandelbrot */
	cl_uint          maxIterations;       /**< paramters of mandelbrot */
	cl::Context            context;       /**< CL context */
	cl::vector<cl::Device> devices;       /**< CL device list */
	cl::Buffer        outputBuffer;       /**< CL memory buffer */
	cl::CommandQueue  commandQueue;       /**< CL command queue */
	cl::Program            program;       /**< CL program  */
	cl::Kernel              kernel;       /**< CL kernel */
	cl_int                   width;       /**< width of the output image */
	cl_int                  height;       /**< height of the output image */
	size_t     kernelWorkGroupSize;       /**< Group Size returned by kernel */
	int                 iterations;       /**< Number of iterations for kernel execution */

public:
	cl_int *output;                       /**< Output array */

public:
	/** 
	* Constructor 
	* Initialize member variables
	*/
	GameOfLife(float POPULATION) : population(POPULATION) {
		seed = 123;
		output = NULL;
		maxIterations = 100;
		width = 100;
		height = width;
		scale = 4.0f;
		iterations = 100;
	}

	/**
	* setup host/device memory and OpenCL
	* @return 0 on success and -1 on failure
	*/
	int setup();

	/**
	* Allocate and initialize host memory array with random values
	* @return 0 on success and -1 on failure
	*/
	int setupGameOfLife();

	/**
	* OpenCL related initialisations. 
	* Set up Context, Device list, Command Queue, Memory buffers
	* Build CL kernel program executable
	* @return 0 on success and -1 on failure
	*/
	int setupCL();

	/**
	* run OpenCL GameOfLife
	* @return 0 on success and -1 on failure
	*/
	int run();

	/**
	* Set values for kernels' arguments, enqueue calls to the kernels
	* on to the command queue, wait till end of kernel execution.
	* @return 0 on success and -1 on failure
	*/
	int runCLKernels();

	/**
	* cleanup memory allocations
	* @return 0 on success and -1 on failure
	*/
	int cleanup();

	/**
	* get window width
	* @return width
	*/
	cl_uint getWidth(void);

	/**
	* get window height
	* @return height
	*/
	cl_uint getHeight(void);

	/**
	* get pixels to be displayed
	* @return output
	*/
	cl_int* getPixels(void);
};

#endif