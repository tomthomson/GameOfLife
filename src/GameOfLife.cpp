#include "../inc/GameOfLife.hpp"
using namespace std;

int GameOfLife::setRule(char *_rule) {
	int counter = 0;
	unsigned int delimiterPos = 0;
	for (unsigned int i = 0; i < strlen(_rule); i++) {
		if (_rule[i] == '/') {
			counter++;
			delimiterPos = i;
		}
	}
	if (counter != 1) return -1;	/* Only 1 delimiter allowed */
	
	/* Allocate space for rules and set to default value 0 */
	rulesSizeBytes = 18*sizeof(char);
	rules = (unsigned char*)malloc(rulesSizeBytes);
	memset(rules,0,18);
	
	/* Split up rule in survival and birth */
	std::string splitter(_rule);
	humanRules.push_back('S');
	char numChar[2];
	if (delimiterPos > 0) {
		/* there is a survival definition */
		for (unsigned int i = 0; i < delimiterPos; i++) {
			int number = atoi(splitter.substr(i,1).c_str());
			rules[9+(number==9?0:number)] = 255;
			snprintf(numChar,2,"%i",number);
			humanRules.push_back(numChar[0]);
		}
	}
	humanRules.push_back('/');
	humanRules.push_back('B');
	if (delimiterPos < splitter.size()-1) {
		/* there is a birth definition */
		for (unsigned int i = delimiterPos+1; i < splitter.size(); i++) {
			int number = atoi(splitter.substr(i,1).c_str());
			rules[(number==9?0:number)] = 255;
			snprintf(numChar,2,"%i",number);
			humanRules.push_back(numChar[0]);
		}
	}
	
	return 0;
}

int GameOfLife::setup() {
	if (setupHost() != 0)
		return -1;
	
	if (setupDevice() != 0)
		return -1;
	
	return 0;
}

int GameOfLife::setupHost() {
	rowPitch = imageSize[0]*sizeof(char)*4;
	imageSizeBytes = imageSize[1]*rowPitch;
	origin[0]=0;
	origin[1]=0;
	origin[2]=0;
	region[0]=imageSize[0];
	region[1]=imageSize[1];
	region[2]=1;
	
	startingImage = (unsigned char *)malloc(imageSizeBytes);
	if (startingImage == NULL)
		return -1;
	
	imageA = (unsigned char *)malloc(imageSizeBytes);
	if (imageA == NULL)
		return -1;
	
	imageB = (unsigned char *)malloc(imageSizeBytes);
	if (imageB == NULL)
		return -1;
		
	/* Read population from file */
	if (spawnMode && readPopulation() != 0) return -1;
	
	/* Spawn initial population */
	if (spawnPopulation() != 0) return -1;

	return 0;
}

int GameOfLife::readPopulation() {
	/* Parse file */
	int status = patternFile.parse();
	if (status != 0) {
		switch (status) {
			default: cerr << "Pattern file parse error\n" << endl; break;
			case -2: cerr << "Cannot open file\n" << endl; break;
		}
		
		return -1;
	}
	
	/* Overwrite rule if specified in file, else skip */
	vector<int> birthRules = patternFile.getBirthRules();
	vector<int> survivalRules = patternFile.getSurvivalRules();
	if (birthRules.size() > 0 && survivalRules.size() > 0) {
		/* Reset rule definitions */
		memset(rules,0,18);
		humanRules.clear();
		
		/* Write new definitions */
		char numChar[2];
		humanRules.push_back('S');
		for (unsigned int i = 0; i < survivalRules.size(); i++) {
			rules[9+survivalRules.at(i)] = 255;
			snprintf(numChar,2,"%i",survivalRules.at(i));
			humanRules.push_back(numChar[0]);
		}
		humanRules.push_back('/');
		humanRules.push_back('B');
		for (unsigned int i = 0; i < birthRules.size(); i++) {
			rules[birthRules.at(i)] = 255;
			snprintf(numChar,2,"%i",birthRules.at(i));
			humanRules.push_back(numChar[0]);
		}
	}
	
	return 0;
}

int GameOfLife::spawnPopulation() {
	if (spawnMode) {	/* Spawn population from file pattern */
		return spawnStaticPopulation();
	} else {			/* Spawn random population with given density */
		return spawnRandomPopulation();
	}
}

int GameOfLife::spawnRandomPopulation() {
	int random;
	srand(time(NULL));
	for (int x = 0; x < imageSize[0]; x++) {
		for (int y = 0; y < imageSize[1]; y++) {
			random = rand() % 100;
			if ((float)random / 100.0f < population)
				setState(x, y, ALIVE, startingImage);
			else
				setState(x, y, DEAD, startingImage);
		}
	}
	
	memcpy(imageA, startingImage, imageSizeBytes);
	
	return 0;
}

int GameOfLife::spawnStaticPopulation() {
	int patternWidth = patternFile.getWidth();
	int patternHeight = patternFile.getHeight();
	
	/* Check if pattern fits into image */
	if (imageSize[0] < patternWidth || imageSize[1] < patternHeight) {
		cerr << "Size of pattern (" << patternWidth << "x" << patternHeight;
		cerr << ") bigger than size of board (" << imageSize[0] << "x" << imageSize[1] << ") !" << endl;
		return -1;
	}
	
	/* Spawn pattern in the center of the image */
	int topLeft[2] = {imageSize[0]/2-patternWidth/2,
					  imageSize[1]/2-patternHeight/2};
	/* counter for copied pattern lines */
	int copyLine = 0;
	
	for (int y = 0; y < imageSize[1]; y++) {
		for (int x = 0; x < imageSize[0]; x++) {
			
			if (x == topLeft[0] && y >= topLeft[1]
				&& y < (topLeft[1]+patternHeight)
				) {
				/* Insert pattern line by line into image */
				memcpy(&(startingImage[4*x + (4*imageSize[0]*y)]),
				       &((patternFile.getPattern())[4*patternWidth*copyLine]),
					   4*patternWidth*sizeof(char));
				x += patternWidth;
				copyLine++;
			} else {
				setState(x, y, DEAD, startingImage);
			}
			
		}
	}
	
	memcpy(imageA, startingImage, imageSizeBytes);
	
	return 0;
}

int GameOfLife::setupDevice(void) {
	cl_int status = CL_SUCCESS;
	const char *kernelFile = "kernels.cl";
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
	 * Else pass a NULL and use a default device.
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
	
	/**
	* Create OpenCL command queue with profiling support
	*/
	commandQueue = clCreateCommandQueue(context, devices[0],
							CL_QUEUE_PROFILING_ENABLE, &status);
	assert(status == CL_SUCCESS);
	
	/**
	* Allocate device memory
	*/
	cl_image_format format;
	format.image_channel_order = CL_RGBA;
	format.image_channel_data_type = CL_UNSIGNED_INT8;
	// imageA (texture memory)
	deviceImageA = clCreateImage2D(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
		&format, imageSize[0], imageSize[1], rowPitch, imageA, &status);
	assert(status == CL_SUCCESS);
	// imageB (texture memory)
	deviceImageB = clCreateImage2D(context, CL_MEM_READ_WRITE,
		&format, imageSize[0], imageSize[1], 0, NULL, &status);
	assert(status == CL_SUCCESS);
	// rules (constant memory)
	deviceRules = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
			rulesSizeBytes, rules, &status);
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
		cerr << "Could not load CL source code from file " << kernelFile << endl;
		return -1;
	}
	const char* source = kernels.source().c_str();
	size_t sourceSize[] = {strlen(source)};		
	
	program = clCreateProgramWithSource(context, 1, &source,sourceSize, &status);
	
	/* Create a OpenCL program executable for all the devices specified */
	status = clBuildProgram(program, 1, devices, kernelBuildOptions.c_str(), NULL, NULL);
	
	if (status != CL_SUCCESS) {
		/* if clBuildProgram failed get the build log for the first device */
		char *buildLog;
		size_t buildLogSize;
		/* Get size of build log */
		clGetProgramBuildInfo(program, devices[0],
				CL_PROGRAM_BUILD_LOG, 0, NULL, &buildLogSize);
		/* Allocate space for build log */
		buildLog = new char[buildLogSize+1];
		clGetProgramBuildInfo(program, devices[0],
				CL_PROGRAM_BUILD_LOG, buildLogSize, buildLog, NULL);
		/* to be carefully, terminate with \0 */
		buildLog[buildLogSize] = '\0';
		
		cerr << "\nBUILD LOG:\n" << buildLog << endl;
		
		delete[] buildLog;
		return -1;
	}
	
	/* Get a kernel object handle for the specified kernel */
	kernel = clCreateKernel(program, "nextGeneration", &status);
	assert(status == CL_SUCCESS);
	
	/* Set kernel arguments */
	status |= clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&deviceImageA);
	status |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&deviceImageB);
	status |= clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&deviceRules);
	if (test)
		status |= clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&testBuf);
	assert(status == CL_SUCCESS);
	
	/* Set optimal values for local and global threads */
	size_t maxWorkGroupSize;
	clGetDeviceInfo(devices[0], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), 
					(void*)&maxWorkGroupSize, NULL);
	
	size_t optWorkGroupSize[3];
	clGetKernelWorkGroupInfo(kernel, devices[0],
		CL_KERNEL_COMPILE_WORK_GROUP_SIZE, 3*sizeof(size_t),
		&optWorkGroupSize, NULL);
	
	localThreads[0] = optWorkGroupSize[0];
	localThreads[1] = optWorkGroupSize[1];
	assert(maxWorkGroupSize >= (localThreads[0] * localThreads[1]));
	
	int r1 = imageSize[0] % localThreads[0];
	int r2 = imageSize[1] % localThreads[1];
	globalThreads[0] = (r1 == 0) ? imageSize[0] : imageSize[0] + localThreads[0] - r1;
	globalThreads[1] = (r2 == 0) ? imageSize[1] : imageSize[1] + localThreads[1] - r2;
	
	char threads[32];
	kernelInfo.append(" | blocks: ");
	snprintf(threads,countDigits(globalThreads[0]/localThreads[0])+1,"%i",globalThreads[0]/localThreads[0]);
	kernelInfo.append(threads);
	kernelInfo.append("x");
	snprintf(threads,countDigits(globalThreads[1]/localThreads[1])+1,"%i",globalThreads[1]/localThreads[1]);
	kernelInfo.append(threads);
	kernelInfo.append(" | threads: ");
	snprintf(threads,countDigits(localThreads[0])+1,"%i",localThreads[0]);
	kernelInfo.append(threads);
	kernelInfo.append("x");
	snprintf(threads,countDigits(localThreads[1])+1,"%i",localThreads[1]);
	kernelInfo.append(threads);
	return 0;
}

int GameOfLife::nextGeneration(unsigned char *bufferImage) {
	if (CPUMode)
		return nextGenerationCPU(bufferImage);
	else
		return nextGenerationOpenCL(bufferImage);
}

int GameOfLife::nextGenerationOpenCL(unsigned char *bufferImage) {
	cl_int status = CL_SUCCESS;
	cl_event kernelEvent = NULL;
	cl_event copyEvent = NULL;
	cl_int copyFinished;
	generationsPerCopyEvent = 0;
	
	/* 
	 * Calculate next generations until copying the first
	 * generation from device to host has finished
	 */
	do {
		/* Enqueue a kernel run call and wait for kernel to finish */
		status = clEnqueueNDRangeKernel(commandQueue, kernel, 2, NULL,
			globalThreads, localThreads, NULL, NULL, &kernelEvent);
		clWaitForEvents(1, &kernelEvent);
		assert(status == CL_SUCCESS);
		
		/* Update generation counter */
		generations++;
		generationsPerCopyEvent++;
		
		/* Calculate kernel execution time */
		if (copyEvent == NULL) {
			cl_ulong start, end;
			
			status |= clGetEventProfilingInfo(kernelEvent,
				CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
			
			status |= clGetEventProfilingInfo(kernelEvent,
				CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
			assert(status == CL_SUCCESS);
			
			executionTime = (end - start) * 1.0e-6f;
		}
		clReleaseEvent(kernelEvent);
		
		/* Exchange images for current and next generation */
		status |= clSetKernelArg(kernel,
					0, sizeof(cl_mem),
					switchImages ? (void *)&deviceImageB : (void *)&deviceImageA );
		status |= clSetKernelArg(kernel,
					1, sizeof(cl_mem),
					switchImages ? (void *)&deviceImageA : (void *)&deviceImageB );
		
		/*
		 * Update image on host for OpenGL output
		 * This starts the copy event
		 */
		if (copyEvent == NULL) {
			status |= clEnqueueReadImage(commandQueue,
				switchImages ? deviceImageB : deviceImageA, readSync,
				origin, region, rowPitch, 0, bufferImage,
				NULL, NULL, &copyEvent);
			assert(status == CL_SUCCESS);
		}
		
		switchImages = !switchImages;
		
		/* Get status of copy event */
		status = clGetEventInfo(
			copyEvent, CL_EVENT_COMMAND_EXECUTION_STATUS,
			sizeof(cl_int), &copyFinished,
			NULL);
		assert(status == CL_SUCCESS);
		
		/* Output test vector */
		if (test) {
			status = clEnqueueReadBuffer(commandQueue, testBuf, CL_TRUE,
				0, testSizeBytes, testVec, NULL, NULL, NULL);
			assert(status == CL_SUCCESS);
			
			for (int i = 0; i < 20; i++)
				cout << (i%10) << "|";
			for (int i = 0; i < 11*20; i++) {
				if (i%20==0)
					cout << endl;
				cout << (testVec[i]==0?' ':'X') << "|";
			}
			cout << endl << endl;
		}
	
	} while (copyFinished != CL_COMPLETE);
	clReleaseEvent(copyEvent);
	
	/* Single generation mode */
	if (singleGen)
		switchPause();

	return 0;
}

int GameOfLife::nextGenerationCPU(unsigned char *bufferImage) {
	int n;
	unsigned char state;
	
	#ifdef WIN32
		LARGE_INTEGER frequency;	/* ticks per second */
		LARGE_INTEGER start;
		LARGE_INTEGER end;
	#else
		timeval start;
		timeval end;
	#endif
	
	/* Start timer */
	#ifdef WIN32
		QueryPerformanceFrequency(&frequency);
		QueryPerformanceCounter(&start);
	#else
		gettimeofday(&start, NULL);
	#endif
	
	/* Calculate next generation for each pixel */
	for (int x = 0; x < imageSize[0]; x++) {
		for (int y = 0; y < imageSize[1]; y++) {
			n = getNumberOfNeighbours(x, y, switchImages?imageA:imageB);
			state = getState(x, y, switchImages?imageA:imageB);
			
			if (state == ALIVE) {
				if ((n < 2) || (n > 3))
					setState(x, y, DEAD, switchImages?imageB:imageA);
				else
					setState(x, y, ALIVE, switchImages?imageB:imageA);
			} else {
				if (n == 3)
					setState(x, y, ALIVE, switchImages?imageB:imageA);
				else
					setState(x, y, DEAD, switchImages?imageB:imageA);
			}
		}
	}
	
	/* Stop timer */
	#ifdef WIN32
		QueryPerformanceCounter(&end);
	#else
		gettimeofday(&end, NULL);
	#endif
	
	/* Calculate execution time for one generation */
	#ifdef WIN32
		executionTime = endCount.QuadPart * (1000.0f / frequency.QuadPart)
						+ startCount.QuadPart * (1000.0f / frequency.QuadPart);
	#else
		executionTime = (float)(end.tv_sec - start.tv_sec) * 1000.0f
						+ (float)(end.tv_usec - start.tv_usec) / 1000.0f;
	#endif
	
	/* Update generation counter */
	generations++;
	
	/* Update image for OpenGL output directly on the mapped buffer */
	memcpy(bufferImage, switchImages?imageB:imageA, imageSizeBytes);
	
	switchImages = !switchImages;
	
	/* Single generation mode */
	if (singleGen)
		switchPause();
	
	return 0;
}

int GameOfLife::getNumberOfNeighbours(const int x, const int y, const unsigned char *image) {
	int counter = 0;
	int neighbourCoord[2];

	for (int i = -1; i <= 1; i++) {
		for (int k = -1; k <= 1; k++) {
			neighbourCoord[0] = x + i;
			neighbourCoord[1] = y + k;
			if (!((i == 0) && (k == 0))
				&& (neighbourCoord[0] < imageSize[0])
				&& (neighbourCoord[1] < imageSize[1])
				&& (x + i > 0) && (y + k > 0))
				{
				if (getState(neighbourCoord[0], neighbourCoord[1], image) == ALIVE)
					counter++;
			}
		}
	}

	return counter;
}

int GameOfLife::resetGame(unsigned char *bufferImage) {
	/* Reset host */
	memcpy(imageA, startingImage, imageSizeBytes);
	generations = 0;
	generationsPerCopyEvent = 0;
	executionTime = 0.0f;
	/* Reset device */
	cl_int status = clEnqueueWriteImage(commandQueue,
						deviceImageA, CL_TRUE, origin, region, rowPitch, 0,
						startingImage, NULL, NULL, NULL);
	status |= clSetKernelArg(kernel, 0, sizeof(cl_mem),(void *)&deviceImageA);
	status |= clSetKernelArg(kernel, 1, sizeof(cl_mem),(void *)&deviceImageB);
	assert(status == CL_SUCCESS);
	
	/* Update OpenGL buffer image */
	memcpy(bufferImage, startingImage, imageSizeBytes);
	
	switchImages = true;
	return 0;
}

int GameOfLife::freeMem() {
	/* Releases OpenCL resources */
	cl_int status = CL_SUCCESS;
	if (kernel) {
		status = clReleaseKernel(kernel);
		assert(status == CL_SUCCESS);
	}
	if (program) {
		status = clReleaseProgram(program);
		assert(status == CL_SUCCESS);
	}
	if (deviceImageA) {
		status = clReleaseMemObject(deviceImageA);
		assert(status == CL_SUCCESS);
	}
	if (deviceImageB) {
		status = clReleaseMemObject(deviceImageB);
		assert(status == CL_SUCCESS);
	}
	if (deviceRules) {
		status = clReleaseMemObject(deviceRules);
		assert(status == CL_SUCCESS);
	}
	if (commandQueue) {
		status = clReleaseCommandQueue(commandQueue);
		assert(status == CL_SUCCESS);
	}
	if (context) {
		status = clReleaseContext(context);
		assert(status == CL_SUCCESS);
	}
	if (test) {
		status = clReleaseMemObject(testBuf);
		assert(status == CL_SUCCESS);
	}
	
	/* Release host resources */
	if (startingImage)
		free(startingImage);
	if (imageA)
		free(imageA);
	if (imageB)
		free(imageB);
	if (devices)
		free(devices);
	if (rules) {
		free(rules);
	}
	
	return 0;
}
