#ifndef GAMEOFLIFE_H_
#define GAMEOFLIFE_H_

#include <string.h>
#include <cstdio>
#include <iostream>
#include <cassert>					/* for assert() */
#include <ctime>					/* for time() */
#include <cstdlib>					/* for srand() and rand() */
#ifdef WIN32					// Windows system specific
	#include <windows.h>			/* for QueryPerformanceCounter */
#else							// Unix based system specific
	#include <sys/time.h>			/* for gettimeofday() */
#endif
#include <CL/cl.h>					/* OpenCL definitions */
#include "../inc/KernelFile.hpp"	/* for reading OpenCL kernel files */

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
	unsigned char           *image;    /**< image on the host for current generation */
	unsigned char    *nextGenImage;    /**< temp-image for CPU calculation */
	size_t          imageSizeBytes;    /**< size of image in bytes */
	bool                        ab;    /**< switch for image exchange */

	bool                 useOpenCL;    /**< CPU/OpenCL switch for calculating next generation*/
	bool                    paused;    /**< start/stop calculation of next generation */
	bool                 singleGen;    /**< switch for single generation mode */
	float            executionTime;    /**< execution time for calculation of 1 generation */
	int                timerOutput;
	
	cl_context             context;    /**< CL context */
	cl_device_id          *devices;    /**< CL device list */
	cl_mem            deviceImageA;    /**< CL image object for first image */
	cl_mem            deviceImageB;    /**< CL image object for second image */
	size_t                rowPitch;    /**< CL row pitch for image objects */
	cl_command_queue  commandQueue;    /**< CL command queue */
	cl_program             program;    /**< CL program  */
	cl_kernel               kernel;    /**< CL kernel */
	size_t        globalThreads[2];    /**< CL total number of work items for a kernel */
	size_t         localThreads[2];    /**< CL number of work items per group */
	
	float                 *testVec;
	cl_mem                 testBuf;
	size_t           testSizeBytes;
	bool                      test;

public:
	/** 
	* Constructor.
	* Initialize member variables
	*/
	GameOfLife(unsigned char r, float p, int w, int h)
			: rules(r), population(p), width(w), height(h),
			image(NULL), nextGenImage(NULL), useOpenCL(true),
			paused(true), singleGen(false), ab(true),
			timerOutput(10)
		{
			rowPitch = w*sizeof(char)*4;
			imageSizeBytes = h*rowPitch;
			
			test = false;
			testSizeBytes = 60*sizeof(float);
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
	int nextGeneration(unsigned char* bufferImage);

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

	/**
	* Switch single generation mode on/off.
	*/
	void singleGeneration() {
		singleGen = !singleGen;
		std::cout << "Single generation mode "
				  << (singleGen ? "on":"off")
				  << "." << std::endl;
	}
	
	/**
	* Get execution time of current generation.
	* @return executionTime
	*/
	float getExecutionTime() {
		return executionTime;
	}

	/**
	* Get image of current generation.
	* @return image
	*/
	unsigned char * getImage() {
		return image;
	}

	/**
	* Set state if OpenCL is used for calculating next generation
	* @param state state of OpenCL usage
	*/
	void setOpenCL(bool state) {
		useOpenCL = state;
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
	int nextGenerationOpenCL(unsigned char* bufferImage);
	
	/**
	* Calculate next generation with CPU.
	* @return 0 on success and -1 on failure
	*/
	int nextGenerationCPU(unsigned char* bufferImage);
	
	/**
	* Calculate the number of neighbours for a cell.
	* @param x x coordinate of cell
	* @param y y coordinate of cell
	* @return number of neighbours
	*/
	int getNumberOfNeighbours(const int x, const int y);

	/**
	* Set the state of a cell.
	* @param x x coordinate of cell
	* @param y y coordinate of cell
	* @param state new state of cell
	* @param image update state in this image
	*/
	void setState(int x, int y, unsigned char state, unsigned char *image) {
		image[4*x + (4*width*y)] = state;
		image[(4*x+1) + (4*width*y)] = state;
		image[(4*x+2) + (4*width*y)] = state;
		image[(4*x+3) + (4*width*y)] = 1;
	}

	/**
	* Get the state of a cell.
	* @param x and y coordinate of cell
	* @return state
	*/
	unsigned char getState(int x, int y) {
		return (image[4*x + (4*width*y)]);
	}
};

#endif
