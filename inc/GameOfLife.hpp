#ifndef GAMEOFLIFE_H_
#define GAMEOFLIFE_H_

#include <cstdio>
#include <iostream>
#include <fstream>
#include <cassert>       /* for assert() */
#include <ctime>         /* for time() */
#include <unistd.h>
#include <sys/time.h>
#include <cstdlib>       /* for srand() and rand() */
#include "../inc/KernelFile.hpp"
/* OpenCL definitions */
#define __CL_ENABLE_EXCEPTIONS
#define __NO_STD_VECTOR
#include <CL/cl.hpp>
#pragma OPENCL EXTENSION cl_khr_gl_sharing : enable /* enable OpenGL sharing */
#define cl_khr_gl_sharing
#include <CL/cl_gl.h>    /* OpenCL/OpenGL interoperation */

/* global definition of live and dead state */
#define ALIVE 255
#define DEAD 0

class GameOfLife {
private:
	unsigned char            rules;    /**< rules for calculating next generation */
	float               population;    /**< chance to create new individual when
	                                        using random starting population */
	int                      width;    /**< width of image */
	int                     height;    /**< height of image */
	unsigned char    *nextGenImage;    /**< temp-image for CPU calculation */
	bool                        ab;    /**< switch for image exchange */

	bool                    paused;    /**< start/stop calculation of next generation */

	cl::Context            context;    /**< CL context */
	cl::vector<cl::Device> devices;    /**< CL device list */
	cl::Image2D       deviceImageA;    /**< CL image object for first image */
	cl::Image2D       deviceImageB;    /**< CL image object for second image */
	size_t                rowPitch;    /**< CL row pitch for image objects */
	cl::CommandQueue  commandQueue;    /**< CL command queue */
	cl::Program            program;    /**< CL program  */
	cl::Kernel              kernel;    /**< CL kernel */
	
	float                 *testVec;
	cl::Buffer             testBuf;
	size_t                testSize;
	bool                      test;

	float average;
	int counter;

public:
	bool                 useOpenCL;    /**< CPU/OpenCL switch for calculating next generation*/
	unsigned char           *image;    /**< image on the host that is displayed with OpenGL */
	size_t          imageSizeBytes;    /**< size of image in bytes */

public:
	/** 
	* Constructor.
	* Initialize member variables
	*/
	GameOfLife(unsigned char r, float p, int w, int h)
			: rules(r), population(p), width(w), height(h),
			image(NULL), nextGenImage(NULL), useOpenCL(true), paused(true)
		{
			imageSizeBytes = w*h*sizeof(char)*4;
			rowPitch = w*sizeof(char)*4;
			ab = true;
			
			test = false;
			if (test)
				testSize = 60*sizeof(float);

			average = 0.0f;
			counter = 0;
	}

	/**
	* Setup host/device memory and OpenCL.
	* @return 0 on success and -1 on failure
	*/
	int setup();

	/**
	* Calculate next generation.
	* @return 0 on success and -1 on failure
	*/
	int nextGeneration();

	/**
	* Get the state of a cell.
	* @return state
	*/
	unsigned char getState(int x, int y) {
		return (image[4*x + (4*width*y)]);
	}

	/**
	* Free memory.
	* @return 0 on success and -1 on failure
	*/
	int freeMem();

	/**
	* Get running state of GameOfLife.
	* @return paused
	*/
	bool isPaused() {
		return paused;
	}

	/**
	* Start/stop calculation of next generation.
	*/
	void pause() {
		paused = !paused;
		std::cout << (paused ? "Stopping ":"Starting ")
				<< "calculation of next generation." << std::endl;
	}

private:

	/**
	* Host initialisations.
	* Allocate and initialize host image
	* @return 0 on success and -1 on failure
	*/
	int setupHost();

	/**
	* Device initialisations.
	* Set up context, device list, command queue, memory buffers
	* Allocate device images
	* Build CL kernel program executable
	* @return 0 on success and -1 on failure
	*/
	int setupDevice();

	/**
	* Spawn initial population.
	*/
	void spawnPopulation();
	
	/**
	* Spawn random population.
	*/
	void spawnRandomPopulation();
	
	/**
	* Spawn predefined population.
	*/
	void spawnStaticPopulation();

	/**
	* Calculate next generation with OpenCL.
	* @return 0 on success and -1 on failure
	*/
	int nextGenerationOpenCL();
	
	/**
	* Calculate next generation with CPU.
	* @return 0 on success and -1 on failure
	*/
	int nextGenerationCPU();
	
	/**
	* Calculate the number of neighbours for a cell.
	* @return number of neighbours
	*/
	int getNumberOfNeighbours(const int x, const int y);

	/**
	* Set the state of a cell.
	*/
	void setState(int x, int y, unsigned char state, unsigned char *image) {
		image[4*x + (4*width*y)] = state;
		image[(4*x+1) + (4*width*y)] = state;
		image[(4*x+2) + (4*width*y)] = state;
		image[(4*x+3) + (4*width*y)] = 1;
	}
};

#endif
