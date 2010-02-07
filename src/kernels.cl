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

__kernel
__attribute__( (reqd_work_group_size(24, 16, 1)) )
//__attribute__( (reqd_work_group_size(13, 32, 1)) )
	void nextGeneration(
		__read_only image2d_t imageA,
		__write_only image2d_t imageB,
		__constant uchar *rules
		//,__global float *test
		) {
	/* Get image dimensions */
	__private int2 imageDim = get_image_dim(imageA);
	/* Get coordinates of current cell */
	__private int2 coord = (int2)(get_global_id(0),get_global_id(1));
	
	/* Only valid coordinates calculate next generation */
	if (!(coord.x<imageDim.x) || !(coord.y<imageDim.y))
		return;
	
	/* Calculate normalized coords which are required for read_imageui */
	__private float2 coordNormalized = (float2)((float)coord.x/(float)imageDim.x,
									  (float)coord.y/(float)imageDim.y);
	
	/* Get state of current cell from current generation (imageA) */
	__private uint4 state = getState(coordNormalized, imageA);
	/* Get number of neighbours of current cell from current generation (imageA)*/
	__private uchar numberOfNeighbours =
		getNumberOfNeighbours(state, coordNormalized, imageDim, imageA);
	
	/* Write state of cell in next generation to imageB according to rules */
	__private uchar i = numberOfNeighbours + 9*(state.x >> 7);
	setState(coord, (uint4)(rules[i],rules[i],rules[i],1), imageB);
	
	/* Write test output */
	//if (coord.x >= 22 && coord.x < 42 && coord.y >= 33 && coord.y <= 36)
		//test[coord.x-22 + (coord.y-33)*20] = coordNormalized.x;
}
