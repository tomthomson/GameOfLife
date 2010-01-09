#ifndef GAMEOFLIFE_H_
#define GAMEOFLIFE_H_

#include <cassert>
#include <ctime> // For time()
#include <cstdlib> // For srand() and rand()
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
	float               population;       /**< starting population density */
	int                      width;       /**< width of image */
	int                     height;       /**< height of image */
	size_t          imageSizeBytes;       /**< size of image in bytes */

	cl::Context            context;       /**< CL context */
	cl::vector<cl::Device> devices;       /**< CL device list */
	cl::Buffer        deviceImageA;       /**< CL memory buffer for first image on the device */
	cl::Buffer        deviceImageB;       /**< CL memory buffer for second image on the device */
	cl::CommandQueue  commandQueue;       /**< CL command queue */
	cl::Program            program;       /**< CL program  */
	cl::Kernel              kernel;       /**< CL kernel */
	size_t                   sizeX;       /**< NDRange parameters */
	size_t                   sizeY;       /**< NDRange parameters */
	size_t             globalSizeX;       /**< NDRange parameters */
	size_t             globalSizeY;       /**< NDRange parameters */
	size_t     kernelWorkGroupSize;       /**< CL group Size returned by kernel */

public:
	cl_int *image;                        /**< image on the host */

public:
	/** 
	* Constructor 
	* Initialize member variables
	*/
	GameOfLife(float p, int w, int h)
		: population(p), width(w), height(h), image(NULL) {
		imageSizeBytes = w*h*sizeof(int);
		sizeX = 16;
		sizeY = 16;
		globalSizeX = ((width + sizeX - 1) / sizeX);
		globalSizeY = ((height + sizeY - 1) / sizeY);
	}

	/**
	* Setup host/device memory and OpenCL
	* @return 0 on success and -1 on failure
	*/
	int setup();

	/**
	* Calculate next generation
	* @return 0 on success and -1 on failure
	*/
	int nextGeneration();

	/**
	* Get the state of a cell
	* @return state
	*/
	int getState(int x, int y);

	/**
	* Set the state of a cell
	*/
	void setState(int x, int y, int state);

	/**
	* Free memory
	* @return 0 on success and -1 on failure
	*/
	int freeMem();

private:
	/**
	* Host initialisations.
	* Allocate and initialize host image
	* @return 0 on success and -1 on failure
	*/
	int setupHost();

	/**
	* Device initialisations.
	* Set up Context, Device list, Command Queue, Memory buffers
	* Allocate device images
	* Build CL kernel program executable
	* @return 0 on success and -1 on failure
	*/
	int setupDevice();

	/**
	* Spawn initial population.
	* @return 0 on success and -1 on failure
	*/
	int spawnPopulation();
};

#endif