#include "../inc/GameOfLife.hpp"
using namespace std;

int GameOfLife::setup() {
	if (setupHost() != 0)
		return -1;

	if (setupDevice() != 0)
		return -1;

	return 0;
}

int GameOfLife::setupHost() {
	image = (unsigned char *) malloc(imageSizeBytes);
	if (image == NULL)
		return -1;

	nextGenImage = (unsigned char *) malloc(imageSizeBytes);
	if (nextGenImage == NULL)
		return -1;

	/* Spawn initial population */
	spawnPopulation();

	return 0;
}

void GameOfLife::spawnPopulation() {
	//spawnRandomPopulation();
	spawnStaticPopulation();
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

	try {
		/* Get platform information for a NVIDIA or ATI GPU */
		cl::vector<cl::Platform> platforms;
		cl::Platform::get(&platforms);
		cl::vector<cl::Platform>::iterator pit;

		if (platforms.size() > 0) {
			for (pit = platforms.begin(); pit != platforms.end(); pit++) {
				if (!strcmp((*pit).getInfo<CL_PLATFORM_VENDOR>().c_str(),
						"Advanced Micro Devices, Inc.") ||
						!strcmp((*pit).getInfo<CL_PLATFORM_VENDOR> ().c_str(),
						"NVIDIA Corporation")) {
					break;
				}
			}
		}

		/* 
		 * If there is a NVIDIA or ATI GPU, use it,
		 * else pass a NULL and get whatever the implementation thinks we should be using
		 */
		cl_context_properties cps[3] = { CL_CONTEXT_PLATFORM,
				(cl_context_properties)(*pit)(), 0 };

		/* Creating a context for a platform with specified context properties */
		context = cl::Context(CL_DEVICE_TYPE_GPU, cps, NULL, NULL, &status);
		assert(status == CL_SUCCESS);

		/* Get the list of OpenCL devices associated with context */
		devices = context.getInfo<CL_CONTEXT_DEVICES> ();
		assert(devices.size() > 0);

		/* Test image support of OpenCL device */
		if (!devices[0].getInfo<CL_DEVICE_IMAGE_SUPPORT>())
			throw cl::Error(-666, "This OpenCL device does not support images");

		/* Create command queue */
		commandQueue = cl::CommandQueue(context, devices[0], 0, &status);
		assert(status == CL_SUCCESS);

		/* Allocate the OpenCL image memory objects for the images on the device GMEM */
		cl::ImageFormat format = cl::ImageFormat(CL_R, CL_UNORM_INT8);
		deviceImageA = cl::Image2D(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
			format, width, height, 0, image, &status);
		assert(status == CL_SUCCESS);
		deviceImageB = cl::Image2D(context, CL_MEM_READ_WRITE,
			format, width, height, 0, NULL, &status);
		assert(status == CL_SUCCESS);
		
		testVec = (float*) malloc(10*sizeof(float));
		testBuf = cl::Buffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
			testSize, testVec, &status);
		assert(status == CL_SUCCESS);

		/* Read in the OpenCL kernel from the source file */
		if (!kernels.open(kernelFile)) {
			cerr << "Could not load CL source code" << endl;
			return -1;
		}
		cl::Program::Sources sources(1, make_pair(kernels.source().data(),
				kernels.source().size()));
		program = cl::Program(context, sources);

		/* Create a OpenCL program executable for all the devices specified */
		program.build(devices);

		/* Get a kernel object handle for the specified kernel */
		kernel = cl::Kernel(program, "nextGeneration", &status);
		assert(status == CL_SUCCESS);

		/* Set kernel arguments */
		kernel.setArg(0, deviceImageA);
		kernel.setArg(1, deviceImageB);
		kernel.setArg(2, testBuf);
		
		return 0;
	} catch (cl::Error err) {
		cerr << "ERROR: " << err.what() << "(" << err.err() << ")" << endl;

		/* If clBuildProgram failed get the build log for the first device */
		if (!strcmp("clBuildProgram",err.what())) {
			try {
				cerr << "\nBuild log:\n";
				cerr << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]);;
				cerr << "\n" << endl;
			} catch (cl::Error err) {
				cerr << "ERROR: " << err.what() << "(" << err.err() << ")" << endl;
			}
		}

		return -1;
	}
}

int GameOfLife::nextGeneration(void) {
	if (useOpenCL)
		return nextGenerationOpenCL();
	else
		return nextGenerationCPU();
}

int GameOfLife::nextGenerationOpenCL(void) {
	cl_int status = CL_SUCCESS;
	cl::size_t<3> origin;
	origin.push_back(0);
	origin.push_back(0);
	origin.push_back(0);
	cl::size_t<3> region;
	region.push_back(width);
	region.push_back(height);
	region.push_back(1);

	try {
		/* Enqueue a kernel run call and wait for kernel to finish */
		commandQueue.enqueueNDRangeKernel(kernel, cl::NullRange,
				cl::NDRange(width, height), cl::NullRange);		
		commandQueue.finish();
		
		/* Update first device image */
		commandQueue.enqueueCopyImage(deviceImageB, deviceImageA,
			origin, origin, region, NULL, NULL);
		commandQueue.finish();
		/* second version to switch images */
		/*
		if (ab) {
			kernel.setArg(0, deviceImageB);
			kernel.setArg(1, deviceImageA);
			ab = false;
		} else {
			kernel.setArg(0, deviceImageA);
			kernel.setArg(1, deviceImageB);
			ab = true;
		}
		*/
		
		/* Synchronous (i.e. blocking) read of results */
		commandQueue.enqueueReadImage(deviceImageA, CL_TRUE,
			origin, region, 0, 0, image, NULL, NULL);
		commandQueue.finish();
		
		commandQueue.enqueueReadBuffer(testBuf, CL_TRUE,
			0, testSize, testVec, NULL, NULL);
		commandQueue.finish();
		
		for (int i = 0; i < 10; i++)
			cout << i << ": " << testVec[i] << endl;
		paused = true;

		return 0;
	} catch (cl::Error err) {
		cerr << "ERROR: " << err.what() << "(" << err.err() << ")" << endl;
		return -1;
	}
}

int GameOfLife::nextGenerationCPU(void) {
	int n;
	unsigned char state;

	for (int x = 1; x < width; x++) {
		for (int y = 1; y < height; y++) {
			n = getNumberOfNeighbours(x, y);
			state = getState(x, y);
			
			if (state == ALIVE) {
				if ((n < 2) || (n > 3))
					setState(x, y, DEAD, nextGenImage);
				else
					setState(x, y, ALIVE, nextGenImage);
			} else {
				if (n == 3)
					setState(x, y, ALIVE, nextGenImage);
				else
					setState(x, y, DEAD, nextGenImage);
			}
		}
	}
	
	memcpy(image, nextGenImage, imageSizeBytes);
	return 0;
}

int GameOfLife::getNumberOfNeighbours(const int x, const int y) {
	int counter = 0;

	for (int i = -1; i <= 1; i++) {
		for (int k = -1; k <= 1; k++) {
			if (!((i == 0) && (k == 0)) && (x + i < width) && (y + k < height)
					&& (x + i > 0) && (y + k > 0)) {
				if (getState(x + i, y + k) == ALIVE)
					counter++;
			}
		}
	}

	return counter;
}

int GameOfLife::freeMem() {
	/* Releases OpenCL resources */

	/* Release host resources */
	if (image)
		free( image);
	if (nextGenImage)
		free( nextGenImage);

	return 0;
}
