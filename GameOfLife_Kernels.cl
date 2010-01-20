/* global definition of live and dead state */
__constant uint4 ALIVE = (uint4)(255, 255, 255, 1);
__constant uint4 DEAD = (uint4)(0, 0, 0, 1);
/* properties for reading/writing images */
sampler_t sampler = CLK_NORMALIZED_COORDS_TRUE |
							CLK_ADDRESS_REPEAT | CLK_FILTER_NEAREST;

inline uint4 getState(float2 coord,
			   __read_only image2d_t image
			   ) {
	return read_imageui(image, sampler, coord);
}

inline void setState(int2 coord,uint4 state,
			  __write_only image2d_t image
			  ) {
	write_imageui(image, coord, state);
}

uchar getNumberOfNeighbours(float2 coord, int2 imageDim,
						    __read_only image2d_t image
							) {
	uchar counter = 0;
	float2 neighbourCoord;
	uint4 neighbourState;

	for (int i=-1; i<=1; i++) {
		for (int k=-1; k<=1; k++) {
			neighbourCoord = (float2)((float)coord.x+((float)i/(float)imageDim.x),
									  (float)coord.y+((float)k/(float)imageDim.x));
			if (!((i==0)&&(k==0))) {
				neighbourState = getState(neighbourCoord, image);
				if (neighbourState.x == ALIVE.x)
					counter++;
			}
		}
	}

	return counter;
}

void applyRules(uchar rules, uint4 state, uchar n, int2 coord,
				__write_only image2d_t image
				) {
	switch (rules) {
	case 0:
		if (state.x == ALIVE.x) {
			if ((n < 2) || (n > 3))
				setState(coord, (uint4)(state.x-5, state.y-5, state.z-5, 1), image);
			else
				setState(coord, ALIVE, image);
		} else {
			if (n == 3)
				setState(coord, ALIVE, image);
			else if (state.x > DEAD.x)
				setState(coord, (uint4)(state.x-5, state.y-5, state.z-5, 1), image);
			else
				setState(coord, DEAD, image);
		}
		break;
	}
}

__kernel void nextGeneration(
		__read_only image2d_t imageA,
		__write_only image2d_t imageB,
		__constant uchar rules
		//,__global float *test
	) {
	/* Get image dimensions */
	int2 imageDim = get_image_dim(imageA);
	/* Get coordinates of current cell and calculate normalized values */
	int2 coord = (int2)(get_global_id(0), get_global_id(1));
	float2 coordNormalized = (float2)((float)coord.x/(float)imageDim.x,
									  (float)coord.y/(float)imageDim.y);
	
	/* only valid coordinates calculate next generation */
	if (!(coord.x<imageDim.x) || !(coord.y<imageDim.y))
		return;
	
	/* Get state of current cell */
	uint4 state = getState(coordNormalized, imageA);
	/* Get number of neighbours of current cell */
	uchar numberOfNeighbours = getNumberOfNeighbours(coordNormalized, imageDim, imageA);
	
	/* Write test output */
	//if (coord.x >= 95 && coord.x < 115 && coord.y >= 99 && coord.y <= 101)
		//test[coord.x-95 + (coord.y-99)*20] = coordNormalized.x;
	
	/* Apply rules for next generation */
	applyRules(rules, state, numberOfNeighbours, coord, imageB);
}
