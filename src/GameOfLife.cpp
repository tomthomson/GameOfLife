#include "../inc/GameOfLife.hpp"
using namespace std;

int GameOfLife::setup() {
	if (setupHost() != 0)
		return -1;

	if (useOpenCL && setupDevice() != 0)
		return -1;

	return 0;
}

int GameOfLife::setupHost() {
	image = (unsigned char *)malloc(imageSizeBytes);
	if (image == NULL)
		return -1;

	nextGenImage = (unsigned char *)malloc(imageSizeBytes);
	if (nextGenImage == NULL)
		return -1;

	/* Spawn initial population */
	spawnPopulation();

	return 0;
}

void GameOfLife::spawnPopulation() {
	spawnRandomPopulation();
	//spawnStaticPopulation();
}

void GameOfLife::spawnRandomPopulation() {
	cout << "Spawning population for Game of Life with a chance of "
			<< population << "..." << endl << endl;
	int random;
	srand(time(NULL));
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			random = rand() % 100;
			if ((float) random / 100.0f > population)
				setState(x, y, DEAD, image);
			else
				setState(x, y, ALIVE, image);
		}
	}
}

void GameOfLife::spawnStaticPopulation() {
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			setState(x, y, DEAD, image);
		}
	}
	for (int x = 100; x < 110; x++) {
		setState(x, 100, ALIVE, image);
	}
}

int GameOfLife::setupDevice(void) {
	cl_int status = CL_SUCCESS;
	const char* kernelFile = "GameOfLife_Kernels.cl";
	KernelFile kernels;
	
	/*
	 * From the available platforms use preferably AMD or NVIDIA
	 */
	cl_uint numberOfPlatforms;
	cl_platform_id platform = NULL;
	status = clGetPlatformIDs(0, NULL, &numberOfPlatforms);
	assert(status == CL_SUCCESS);
	
	if(numberOfPlatforms > 0) {
		cl_platform_id* platforms = new cl_platform_id[numberOfPlatforms];
		status = clGetPlatformIDs(numberOfPlatforms, platforms, NULL);
		assert(status == CL_SUCCESS);
		
		for(unsigned int i=0; i < numberOfPlatforms; i++) {
			char vendor[100];
			status = clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR,
						sizeof(vendor), vendor, NULL);
			platform = platforms[i];
			if(!strcmp(vendor, "Advanced Micro Devices, Inc.")
				|| !strcmp(vendor, "NVIDIA Corporation")) {
				break;
			}
		}
		
		delete platforms;
	}

	/* 
	 * If there is a NVIDIA or AMD platform, use it.
	 * Else pass a NULL and get whatever the
	 * implementation thinks we should be using.
	 */
	cl_context_properties cps[3] = { CL_CONTEXT_PLATFORM,
									(cl_context_properties)platform, 0 };
	cl_context_properties* cprops = (NULL == platform) ? NULL : cps;
	
	/**
	* Create context for OpenCL platform with specified context properties
	*/
	context = clCreateContextFromType(cprops, CL_DEVICE_TYPE_GPU, NULL, NULL, &status);
	assert(status == CL_SUCCESS);
	
	/* Get the size of device list data */
	size_t deviceListSize;
	status = clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &deviceListSize);
	assert(status == CL_SUCCESS);
	
	devices = (cl_device_id *)malloc(deviceListSize);
	assert(devices > 0);
	
	/* Get the device list data */
	status = clGetContextInfo(context, CL_CONTEXT_DEVICES, deviceListSize, devices, NULL);
	assert(status == CL_SUCCESS);
	
	/**
	* Check OpenCL device skills
	*/
	/* Check image support */
	cl_bool imageSupport;
	status = clGetDeviceInfo(devices[0], CL_DEVICE_IMAGE_SUPPORT,
								sizeof(cl_bool), &imageSupport, NULL);
	assert(status == CL_SUCCESS && imageSupport == CL_TRUE);
	/* Check OpenGL sharing support */
	/*
	#ifdef cl_khr_sharing
		cout << "cl_khr_sharing supported" << std::endl;
	#else
		throw cl::Error(-667, "This OpenCL device does not support OpenGL sharing");
	#endif
	*/
	
	/**
	* Create OpenCL command queue with profiling support
	*/
	commandQueue = clCreateCommandQueue(context, devices[0],
							CL_QUEUE_PROFILING_ENABLE, &status);
	assert(status == CL_SUCCESS);
	
	/**
	* Allocate device memory
	*/
	/* Allocate image objects in texture memory */
	cl_image_format format;
	format.image_channel_order = CL_RGBA;
	format.image_channel_data_type = CL_UNSIGNED_INT8;
	// imageA		
	deviceImageA = clCreateImage2D(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
		&format, width, height, rowPitch, image, &status);
	assert(status == CL_SUCCESS);
	// imageB
	deviceImageB = clCreateImage2D(context, CL_MEM_READ_WRITE,
		&format, width, height, 0, NULL, &status);
	assert(status == CL_SUCCESS);
	// test
	if (test) {
		testVec = (float *)malloc(testSizeBytes);
		testBuf = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
			testSizeBytes, testVec, &status);
		assert(status == CL_SUCCESS);
	}
	
	/**
	* Load kernel file, build program and create kernel
	*/
	/* Read in the OpenCL kernel from the source file */
	if (!kernels.open(kernelFile)) {
		cerr << "Could not load CL source code" << endl;
		return -1;
	}
	const char* source = kernels.source().c_str();
	size_t sourceSize[] = {strlen(source)};		
	
	program = clCreateProgramWithSource(context, 1, &source,sourceSize, &status);
	
	/* Create a OpenCL program executable for all the devices specified */
	status = clBuildProgram(program, 1, devices, NULL, NULL, NULL);
	if (status != CL_SUCCESS) {
		/* if clBuildProgram failed get the build log for the first device */
		char *build_log;
		size_t ret_val_size;
		
		clGetProgramBuildInfo(program, devices[0],
				CL_PROGRAM_BUILD_LOG, 0, NULL, &ret_val_size);
		
		build_log = new char[ret_val_size+1];
		clGetProgramBuildInfo(program, devices[0],
				CL_PROGRAM_BUILD_LOG, ret_val_size, build_log, NULL);
		// to be carefully, terminate with \0
		build_log[ret_val_size] = '\0';
		
		cerr << "\nBUILD LOG:\n" << build_log << endl;
		
		delete[] build_log;
		return -1;
	}
	
	/* Get a kernel object handle for the specified kernel */
	kernel = clCreateKernel(program, "nextGeneration", &status);
	assert(status == CL_SUCCESS);

	/* Set kernel arguments */
	status |= clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&deviceImageA);
	status |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&deviceImageB);
	status |= clSetKernelArg(kernel, 2, sizeof(char), (void *)&rules);
	if (test)
		status |= clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&testBuf);
	assert(status == CL_SUCCESS);
	
	/* Set optimal values for local and global threads */
	size_t maxWorkGroupSize;
	clGetDeviceInfo(devices[0], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), 
					(void*)&maxWorkGroupSize, NULL);
	localThreads[0] = 24;
	localThreads[1] = 16;
	assert(maxWorkGroupSize >= localThreads[0] * localThreads[1]);
	int r1 = width % localThreads[0];
	int r2 = height % localThreads[1];
	globalThreads[0] = (r1 == 0) ? width : width + localThreads[0] - r1;
	globalThreads[1] = (r2 == 0) ? height : height + localThreads[1] - r2;
	
	return 0;
}

int GameOfLife::nextGeneration(void) {
	if (useOpenCL)
		return nextGenerationOpenCL();
	else
		return nextGenerationCPU();
}

int GameOfLife::nextGenerationOpenCL(void) {
	cl_int status = CL_SUCCESS;
	size_t origin[3] = {0,0,0};
	size_t region[3] = {width,height,1};
	
	/* Enqueue a kernel run call and wait for kernel to finish */
	cl_event kernelEvent;
	status = clEnqueueNDRangeKernel(commandQueue, kernel, 2, NULL,
		globalThreads, localThreads, NULL, NULL, &kernelEvent);
	assert(status == CL_SUCCESS);
	clFinish(commandQueue);

	/* Output kernel execution time */
	/*
	if (timerOutput==10) {
		cl_ulong start, end;
		status = clGetEventProfilingInfo(kernelEvent,
			CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
		assert(status == CL_SUCCESS);
		status = clGetEventProfilingInfo(kernelEvent,
			CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
		assert(status == CL_SUCCESS);
		cout << (end - start) * 1.0e-6f << endl;
		timerOutput = 0;
	} else {
		timerOutput++;
	}
	*/
	clReleaseEvent(kernelEvent);
	
	/* Update first device image */
	/* first way to switch images */
	/*
	status = clEnqueueCopyImage(commandQueue, deviceImageB, deviceImageA,
		origin, origin, region, NULL, NULL, NULL);
	assert(status == CL_SUCCESS);
	*/
	/* Synchronous (i.e. blocking) read of results */
	/*
	status = clEnqueueReadImage(commandQueue, deviceImageA, CL_TRUE,
		origin, region, rowPitch, 0, image, NULL, NULL, NULL);
	assert(status == CL_SUCCESS);
	clFinish(commandQueue);
	*/
	/* second way to switch images */
	
	if (ab) {
		status |= clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&deviceImageB);
		status |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&deviceImageA);
		status |= clEnqueueReadImage(commandQueue, deviceImageB, CL_TRUE,
			origin, region, rowPitch, 0, image, NULL, NULL, NULL);
		assert(status == CL_SUCCESS);
		ab = false;
	} else {
		status |= clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&deviceImageA);
		status |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&deviceImageB);
		status |= clEnqueueReadImage(commandQueue, deviceImageA, CL_TRUE,
			origin, region, rowPitch, 0, image, NULL, NULL, NULL);
		assert(status == CL_SUCCESS);
		ab = true;
	}
	
	/* Output test vector */
	if (test) {
		status = clEnqueueReadBuffer(commandQueue, testBuf, CL_TRUE,
			0, testSizeBytes, testVec, NULL, NULL, NULL);
		assert(status == CL_SUCCESS);
		
		for (int i = 0; i < 20; i++)
			cout << (i%10) << "|";
		for (int i = 0; i < 60; i++) {
			if (i%20==0)
				cout << endl;
			cout << testVec[i] << "|";
		}
		cout << endl;
	}
	
	/* Single generation mode */
	//pause();

	return 0;
}

int GameOfLife::nextGenerationCPU(void) {
	int n;
	unsigned char state;

	timeval start;
    timeval end;

	gettimeofday(&start, 0);
	
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			n = getNumberOfNeighbours(x, y);
			state = getState(x, y);
			
			if (state == ALIVE) {
				if ((n < 2) || (n > 3))
					setState(x, y, state-5, nextGenImage);
				else
					setState(x, y, ALIVE, nextGenImage);
			} else {
				if (n == 3)
					setState(x, y, ALIVE, nextGenImage);
				else
					setState(x, y, state==0?0:state-5, nextGenImage);
			}
		}
	}

	gettimeofday(&end, 0);
	cout << (float)(end.tv_usec - start.tv_usec) / 1000.0f << endl;
	
	memcpy(image, nextGenImage, imageSizeBytes);
	return 0;
}

int GameOfLife::getNumberOfNeighbours(const int x, const int y) {
	int counter = 0;
	int neighbourCoord[2];

	for (int i = -1; i <= 1; i++) {
		for (int k = -1; k <= 1; k++) {
			neighbourCoord[0] = x + i;
			neighbourCoord[1] = y + k;
			if (!((i == 0) && (k == 0))
				&& (neighbourCoord[0] < width)
				&& (neighbourCoord[1] < height)
				&& (x + i > 0) && (y + k > 0))
				{
				if (getState(neighbourCoord[0], neighbourCoord[1]) == ALIVE)
					counter++;
			}
		}
	}

	return counter;
}

int GameOfLife::freeMem() {
	/* Releases OpenCL resources */
	cl_int status = CL_SUCCESS;
	if (useOpenCL) {
		status = clReleaseKernel(kernel);
		assert(status == CL_SUCCESS);
		status = clReleaseProgram(program);
		assert(status == CL_SUCCESS);
		status = clReleaseMemObject(deviceImageA);
		assert(status == CL_SUCCESS);
		status = clReleaseMemObject(deviceImageA);
		assert(status == CL_SUCCESS);
		if (test) {
			status = clReleaseMemObject(testBuf);
			assert(status == CL_SUCCESS);
		}
		status = clReleaseCommandQueue(commandQueue);
		assert(status == CL_SUCCESS);
		status = clReleaseContext(context);
		assert(status == CL_SUCCESS);
	}

	/* Release host resources */
	if (image)
		free(image);
	if (nextGenImage)
		free(nextGenImage);
	if (devices)
		free(devices);
	
	return 0;
}
