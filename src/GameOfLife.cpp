#include "../inc/GameOfLife.hpp"

int GameOfLife::setup() {
	if(setupGameOfLife()!=0)
		return -1;
	if(setupCL()!=0)
		return -1;
	return 0;
}

int GameOfLife::setupGameOfLife() {
	cl_uint sizeBytes;

	/* allocate and init memory used by host */
	sizeBytes = width * height * sizeof(cl_int);
	output = (cl_int *) malloc(sizeBytes);
	if(output==NULL)
		return -1;

	return 0;
}

int GameOfLife::setupCL(void) {
	cl_int status = CL_SUCCESS;
	const char* kernelFile = "GameOfLife_Kernels.cl";
	KernelFile kernels;

	try {
		// Platform info
		cl::vector<cl::Platform> platforms;
		cl::Platform::get(&platforms);

		cl::vector<cl::Platform>::iterator i;
		if(platforms.size() > 0)
		{
			for(i = platforms.begin(); i != platforms.end(); ++i)
			{
				if(!strcmp((*i).getInfo<CL_PLATFORM_VENDOR>().c_str(), "Advanced Micro Devices, Inc."))
				{
					break;
				}
			}
		}

		/* 
		* If we could find our platform, use it. Otherwise pass a NULL and get whatever the
		* implementation thinks we should be using.
		*/
		cl_context_properties cps[3] = { CL_CONTEXT_PLATFORM, (cl_context_properties)(*i)(), 0 };

		// Creating a context AMD platform
		context = cl::Context(CL_DEVICE_TYPE_GPU, cps, NULL, NULL, &status); assert(status == CL_SUCCESS);

		// get the list of GPU devices associated with context
		devices = context.getInfo<CL_CONTEXT_DEVICES>();
		//assert(devices.size() > 0);

		// Create command queue
		commandQueue = cl::CommandQueue(context, devices[0], 0, &status); assert(status == CL_SUCCESS);

		// Allocate the OpenCL buffer memory objects for output on the device GMEM
		outputBuffer = cl::Buffer(context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
			sizeof(cl_float) * width * height, output, &status);
		assert(status == CL_SUCCESS);

		// Reed in the OpenCL kernel from source file		
		if (!kernels.open(kernelFile)) {
			std::cerr << "Couldn't load CL source code\n";
		}
		cl::Program::Sources sources(1, std::make_pair(kernels.source().data(), kernels.source().size()));
		
		

		program = cl::Program(context, sources);
		// create a cl program executable for all the devices specified
		program.build(devices);

		/* get a kernel object handle for a kernel with the given name */
		kernel = cl::Kernel(program, "gameoflife", &status); assert(status == CL_SUCCESS);

		return 0;
	} catch (cl::Error err) {
		std::cerr << "ERROR: " << err.what() << "(" << err.err() << ")" << std::endl;
		//Get the build log for the first device
		/*
		std::cerr << "\nRetrieving build log\n"	<<
			program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0], &status) << std::endl;
		*/
			
		return -1;
	}
}

int GameOfLife::run() {
	std::cout << "Spawning population for Game of Life with a chance of " << population << std::endl;
	std::cout << "-------------------------------------------" << std::endl;

	std::cout << "Executing kernel for " << iterations << " iterations" << std::endl;
	std::cout << "-------------------------------------------" << std::endl;
	for(int i = 0; i < iterations; i++)
	{
		/* Arguments are set and execution call is enqueued on command buffer */
		if(runCLKernels()!=0)
			return -1;
	}

	return 0;
}

int GameOfLife::runCLKernels(void) {
	cl_int status = CL_SUCCESS;
	size_t local_work_size = 1;
	size_t global_work_size = width*height;

	try {
		/* Check group size against kernelWorkGroupSize */
		status = kernel.getWorkGroupInfo(devices[0], CL_KERNEL_WORK_GROUP_SIZE, &kernelWorkGroupSize);
		assert(status == CL_SUCCESS);

		if((cl_uint)(local_work_size) > kernelWorkGroupSize) {
			std::cerr<<"Out of Resources!"<<std::endl;
			std::cerr<<"Group Size specified : "<<local_work_size<<std::endl;
			std::cerr<<"Max Group Size supported on the kernel : "<<kernelWorkGroupSize<<std::endl;
			return -1;
		}

		/* Set appropriate arguments to the kernel */
		kernel.setArg(0,sizeof(cl_mem),(void*) &outputBuffer);
		kernel.setArg(1,sizeof(cl_float),(void*) &scale);
		kernel.setArg(2,sizeof(cl_uint),(void*) &maxIterations);
		kernel.setArg(3,sizeof(cl_int),(void*) &width);


		/* Enqueue a kernel run call and wait for kernel to finish */
		cl::KernelFunctor func = kernel.bind(commandQueue,cl::NDRange(global_work_size),cl::NDRange(local_work_size));
		func().wait();

		// Synchronous and blocking read of results
		commandQueue.enqueueReadBuffer(outputBuffer, CL_TRUE, 0, width * height * sizeof(cl_float), output, NULL, NULL);

		return 0;
	} catch (cl::Error err) {
		std::cerr << "ERROR: " << err.what() << "(" << err.err() << ")" << std::endl;
		return -1;
	}
}

int GameOfLife::cleanup() {
	if(output)
		free(output);

	return 0;
}

cl_uint GameOfLife::getWidth(void) {
	return width;
}

cl_uint GameOfLife::getHeight(void) {
	return height;
}


cl_int* GameOfLife::getPixels(void) {
	return output;
}
