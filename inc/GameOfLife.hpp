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
	unsigned char            rules;  /**< rules for calculating next generation */
	float               population;  /**< chance to create new individual when
	                                        using random starting population */
	int                      width;  /**< width of image */
	int                     height;  /**< height of image */
	unsigned char          *imageA;  /**< first image on the host */
	unsigned char          *imageB;  /**< second image on the host */
	size_t          imageSizeBytes;  /**< size of image in bytes */
	bool              switchImages;  /**< switch for image exchange */

	unsigned long      generations;  /**< number of calculated generations */
	bool                   CPUMode;  /**< CPU/OpenCL switch for calculating next generation */
	bool                    paused;  /**< start/stop calculation of next generation */
	bool                 singleGen;  /**< switch for single generation mode */
	float            executionTime;  /**< execution time for calculation of 1 generation */
	cl_bool               readSync;  /**< switch for synchronous reading of images from device */
	int    generationsPerCopyEvent;  /**< number of executed kernels during 1 read image call */
	
	cl_context             context;  /**< CL context */
	cl_device_id          *devices;  /**< CL device list */
	cl_command_queue  commandQueue;  /**< CL command queue */
	cl_program             program;  /**< CL program  */
	cl_kernel               kernel;  /**< CL kernel */
	size_t        globalThreads[2];  /**< CL total number of work items for a kernel */
	size_t         localThreads[2];  /**< CL number of work items per group */
	
	cl_mem            deviceImageA;  /**< CL image object for first image */
	cl_mem            deviceImageB;  /**< CL image object for second image */
	size_t                rowPitch;  /**< CL row pitch for image objects */
	size_t               origin[3];  /**< CL offset for image operations */
	size_t               region[3];  /**< CL region for image operations */
	
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
			imageA(NULL), imageB(NULL), generations(0), CPUMode(false),
			paused(true), singleGen(false), switchImages(true),
			executionTime(0.0f), readSync(CL_TRUE), generationsPerCopyEvent(0)
		{
			rowPitch = w*sizeof(char)*4;
			imageSizeBytes = h*rowPitch;
			origin[0]=0; origin[1]=0; origin[2]=0;
			region[0]=w; region[1]=h; region[2]=1;
			
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
	* Get CPU Mode.
	* @return CPUMode
	*/
	bool isCPUMode() {
		return CPUMode;
	}
	
	/**
	* Get single generation mode.
	* @return singleGen
	*/
	bool isSingleGeneration() {
		return singleGen;
	}
	
	/**
	* Get readSync.
	* @return readSync
	*/
	bool isReadSync() {
		return readSync;
	}
	
	/**
	* Switch to CPU/OpenCL mode
	*/
	void switchCPUMode() {
		CPUMode = !CPUMode;
		if (generations == 0)
			return;
		
		/* Update first OpenCL/CPU image to last calculated generation */
		if (CPUMode) {  /* Switch from OpenCL to CPU */
			cl_int status = clEnqueueReadImage(commandQueue,
				switchImages ? deviceImageA : deviceImageB,
				CL_TRUE, origin, region, rowPitch, 0,
				switchImages ? imageA : imageB,
				NULL, NULL, NULL);
			assert(status == CL_SUCCESS);
		} else {        /* Switch from CPU to OpenCL */
			cl_int status = clEnqueueWriteImage(commandQueue,
				switchImages ? deviceImageA : deviceImageB,
				CL_TRUE, origin, region, rowPitch, 0,
				switchImages ? imageA : imageB,
				NULL, NULL, NULL);
			assert(status == CL_SUCCESS);
		}
	}

	/**
	* Start/stop calculation of next generation.
	*/
	void switchPause() {
		paused = !paused;
	}

	/**
	* Switch single generation mode on/off.
	*/
	void switchSingleGeneration() {
		singleGen = !singleGen;
	}
	
	/**
	* Switch synchronous reading of images on/off.
	*/
	void switchreadSync() {
		readSync==CL_TRUE?readSync=CL_FALSE:readSync=CL_TRUE;
	}
	
	/**
	* Get mode for calculating next generations.
	* @param mode execution mode as a string
	*/
	void getExecutionMode(char *mode) {
		if (CPUMode)
			sprintf(mode,"CPU");
		else
			sprintf(mode,"OpenCL");
	}
	
	/**
	* Get execution time of current generation.
	* @return executionTime
	*/
	float getExecutionTime() {
		return executionTime;
	}
	
	/**
	* Get number of calculated generations.
	* @return generations
	*/
	int getGenerations() {
		return generations;
	}
	
	/**
	* Get number of executed kernels while
	* copying calculated next generation image
	* from device to host.
	* @return generationsPerCopyEvent
	*/
	int getGenerationsPerCopyEvent() {
		if (CPUMode)
			return 1;
		else
			return generationsPerCopyEvent;
	}

	/**
	* Get image of current generation.
	* @return image
	*/
	unsigned char * getImage() {
		return imageA;
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
	* @param image search in this image
	* @return number of neighbours
	*/
	int getNumberOfNeighbours(const int x, const int y, const unsigned char *image);

	/**
	* Get the state of a cell.
	* @param x and y coordinate of cell
	* @param image get state from this image
	* @return state
	*/
	unsigned char getState(const int x, const int y, const unsigned char *image) {
		return (image[4*x + (4*width*y)]);
	}
	
	/**
	* Set the state of a cell.
	* @param x x coordinate of cell
	* @param y y coordinate of cell
	* @param state new state of cell
	* @param image update state in this image
	*/
	void setState(const int x, const int y, const unsigned char state, unsigned char *image) {
		image[4*x + (4*width*y)] = state;
		image[(4*x+1) + (4*width*y)] = state;
		image[(4*x+2) + (4*width*y)] = state;
		image[(4*x+3) + (4*width*y)] = 1;
	}
};

#endif
