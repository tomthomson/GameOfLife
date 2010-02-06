/* global definition of live and dead state */
__constant uint4 ALIVE = (uint4)(255, 255, 255, 1);__constant uint4 DEAD = (uint4)(0, 0, 0, 1);
/* properties for reading/writing images */
sampler_t sampler = CLK_NORMALIZED_COORDS_TRUE |
							CLK_ADDRESS_REPEAT | CLK_FILTER_NEAREST;

inline uint4 getState(
				__private float2 coord,
				__read_only image2d_t image
				) {
	return read_imageui(image, sampler, coord);
}

inline void setState(
				__private int2 coord,
				__private uint4 state,
				__write_only image2d_t image
				) {
	write_imageui(image, coord, state);
}

uchar getNumberOfNeighbours(
				__private uint4 state,
				__private float2 coord,
				__private int2 imageDim,
				__read_only image2d_t image
				) {
	
	uchar counter = 0;
	float2 neighbourCoord;
	uint4 neighbourState;
	
	for (int i=-1; i<=1; i++) {
		for (int k=-1; k<=1; k++) {
			neighbourCoord =
				(float2)( (float)coord.x+((float)i/(float)imageDim.x),
						  (float)coord.y+((float)k/(float)imageDim.x)
						 );
			neighbourState = getState(neighbourCoord, image);
			counter += (neighbourState.x >> 7);
		}
	}
	
	return (counter - (state.x >> 7));
}

void applyRules(__private uchar rules,
				__private uint4 state,
				__private uchar n,
				__private int2 coord,
				__write_only image2d_t image
				) {
	
	switch (rules) {
	case 0:
		if (state.x == ALIVE.x) {
			if ((n < 2) || (n > 3))
				setState(coord, DEAD, image);
			else
				setState(coord, ALIVE, image);
		} else {
			if (n == 3)
				setState(coord, ALIVE, image);
			else
				setState(coord, DEAD, image);
		}
		break;
	}
}

__kernel
__attribute__( (reqd_work_group_size(13, 32, 1)) )
	void nextGeneration(
		__read_only image2d_t imageA,
		__write_only image2d_t imageB,
		__private uchar rules,
		__private uchar r0,
		__private uchar r1,
		__private uchar r2,
		__private uchar r3,
		__private uchar r4,
		__private uchar r5,
		__private uchar r6,
		__private uchar r7,
		__private uchar r8,
		__private uchar r9,
		__private uchar r10,
		__private uchar r11,
		__private uchar r12,
		__private uchar r13,
		__private uchar r14,
		__private uchar r15,
		__private uchar r16,
		__private uchar r17
		,__global float *test
		) {
	
	/* Get image dimensions */
	__private int2 imageDim = get_image_dim(imageA);
	/* Get coordinates of current cell and calculate normalized values */
	__private int2 coord = (int2)(get_global_id(0),get_global_id(1));
	__private float2 coordNormalized = (float2)((float)coord.x/(float)imageDim.x,
									  (float)coord.y/(float)imageDim.y);
	
	/* only valid coordinates calculate next generation */
	if (!(coord.x<imageDim.x) || !(coord.y<imageDim.y))
		return;
	
	/* Get state of current cell */
	__private uint4 state = getState(coordNormalized, imageA);
	/* Get number of neighbours of current cell */
	__private uchar numberOfNeighbours = 
		getNumberOfNeighbours(state, coordNormalized, imageDim, imageA);
	
	/* Apply rules for next generation */
	__private uint rulez[18] = {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,r16,r17};
	__private uint i = numberOfNeighbours+(state.x >> 7);
	__private uint4 nextState = (uint4)(rulez[i], rulez[i], rulez[i], 1);
	//setState(coord, nextState, imageB);
	
	/* Write test output */
	if (coord.x >= 95 && coord.x < 115 && coord.y >= 99 && coord.y <= 101)
		test[coord.x-95 + (coord.y-99)*20] = r3;
	
	applyRules(rules, state, numberOfNeighbours, coord, imageB);
	
	/*
	if (coord.x == 100 && coord.y == 100 || coord.x == 102 && coord.y == 102)
		setState(coord, ALIVE, imageB);
	else
		setState(coord, DEAD, imageB);
	*/
}
