/* properties for reading/writing images */
__const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE |
							CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;
__const uint4 alive = (uint4)(255, 0, 0, 1);
__const uint4 dead = (uint4)(0, 0, 0, 1);

uint4 getState(__const int2 coord,__read_only image2d_t image) {
	return read_imageui(image, sampler, coord);
}

void setState(__const int2 coord,__const uint4 state, __write_only image2d_t image) {
	write_imageui(image, coord, state);
}

int getNumberOfNeighbours(__const int2 coord, __const int2 imageDim,
						  __read_only image2d_t image
						 ) {
	int counter = 0;
	int2 neighbourCoord;
	uint4 neighbourState;

	for (int i=-1; i<=1; i++) {
		for (int k=-1; k<=1; k++) {
			neighbourCoord = (int2)(coord.x+i, coord.y+k);
			if (!((i==0)&&(k==0))
					&& (neighbourCoord.x > 0)
					&& (neighbourCoord.y > 0)
					&& (neighbourCoord.x < imageDim.x)
					&& (neighbourCoord.y < imageDim.y)
				){
				neighbourState = getState(neighbourCoord, image);
				if (neighbourState.x == alive.x)
					counter++;
			}
		}
	}

	return counter;
}

__kernel void nextGeneration(__read_only image2d_t imageA,
							 __write_only image2d_t imageB,
							 __global float *test
							) {
	int n;
	uint4 state;
	
	int2 coord = (int2)(get_global_id(0), get_global_id(1));
	int2 imageDim = get_image_dim(imageA);
	
	if ((coord.x<imageDim.x) && (coord.y<imageDim.y) && (coord.x>0) && (coord.y>0)) {
	
		n = getNumberOfNeighbours(coord, imageDim, imageA);
		state = getState(coord, imageA);
				
		if (coord.x >= 95 && coord.x < 115 && coord.y >= 99 && coord.y <= 101)
			test[coord.x-95 + (coord.y-99)*20] = state.x;
		
		if (state.x == alive.x) {
			if ((n < 2) || (n > 3))
				setState(coord, dead, imageB);
			else
				setState(coord, alive, imageB);
		} else {
			if (n == 3)
				setState(coord, alive, imageB);
			else
				setState(coord, dead, imageB);
		}
		
		/*
		setState(coord,dead,imageB);
		if (coord.x == 100 && coord.y == 100)
			setState(coord,alive,imageB);
		else 
			setState(coord,dead,imageB);
		*/
	}
}
