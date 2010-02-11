#ifndef TPBX
#define TPBX 32		// work items (threads) per work group (block) X
#endif
#ifndef TPBY
#define TPBY 12		// work items (threads) per work group (block) Y
#endif

/* properties for reading/writing images */
#ifdef CLAMP
sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE |
							CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;
#else
sampler_t sampler = CLK_NORMALIZED_COORDS_TRUE |
							CLK_ADDRESS_REPEAT | CLK_FILTER_NEAREST;
#endif

inline uint4 getState(
			#ifdef CLAMP
				__private int2 coord,
			#else
				__private float2 coord,
			#endif
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
			#ifdef CLAMP
				__private int2 coord,
			#else
				__private float2 coord,
			#endif
				__private int2 imageDim,
				__read_only image2d_t image
				) {
	
	uchar counter = 0;
#ifdef CLAMP
	int2 neighbourCoord;
#else
	float2 neighbourCoord;
#endif	
	uint4 neighbourState;
	
	for (int i=-1; i<=1; i++) {
		for (int k=-1; k<=1; k++) {
		#ifdef CLAMP
			neighbourCoord = (int2)(coord.x+i,coord.y+k);
		#else
			neighbourCoord =
				(float2)( (float)coord.x+((float)i/(float)imageDim.x),
						  (float)coord.y+((float)k/(float)imageDim.x)
						 );
		#endif
			neighbourState = getState(neighbourCoord, image);
			counter += (neighbourState.x >> 7);
		}
	}
	
	return (counter - (state.x >> 7));
}

__kernel
__attribute__( (reqd_work_group_size(TPBX, TPBY, 1)) )
	void nextGeneration(
		__read_only image2d_t imageA,
		__write_only image2d_t imageB,
		__constant uchar *rules
		) {
	
	/* Get image dimensions */
	__private int2 imageDim = get_image_dim(imageA);
	/* Get coordinates of current cell */
	__private int2 coord = (int2)(get_global_id(0),get_global_id(1));
	
	/* Only valid coordinates calculate next generation */
	if (!(coord.x<imageDim.x) || !(coord.y<imageDim.y)) return;
	
#ifdef CLAMP
	/* Get state of current cell from current generation (imageA) */
	__private uint4 state = getState(coord, imageA);
	/* Get number of neighbours of current cell from current generation (imageA)*/
	__private uchar numberOfNeighbours =
				getNumberOfNeighbours(state, coord, imageDim, imageA);
#else
	/* Calculate normalized coords which are required for read_imageui */
	__private float2 coordNormalized = (float2)((float)coord.x/(float)imageDim.x,
									  (float)coord.y/(float)imageDim.y);
	/* Get state of current cell from current generation (imageA) */
	__private uint4 state = getState(coordNormalized, imageA);
	/* Get number of neighbours of current cell from current generation (imageA)*/
	__private uchar numberOfNeighbours =
				getNumberOfNeighbours(state, coordNormalized, imageDim, imageA);
#endif
	/* Write state of cell in next generation to imageB according to rules */
	__private uchar i = numberOfNeighbours + 9*(state.x >> 7);
	setState(coord, (uint4)(rules[i],rules[i],rules[i],1), imageB);
	
}
