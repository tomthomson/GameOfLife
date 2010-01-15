/* properties for reading/writing images */
__const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;

float4 getState(__const int2 coord,__read_only image2d_t image) {
	return read_imagef(image, sampler, coord);
}

void setState(__const int2 coord, __const float4 state,__write_only image2d_t image) {
	write_imagef(image, coord, state);
}

int getNumberOfNeighbours(__const int2 coord, __const int2 imageDim,__read_only image2d_t image) {
	int counter = 0;
	int2 neighbourCoord;

	for (int i=-1; i<=1; i++) {
		for (int k=-1; k<=1; k++) {
			neighbourCoord = (int2)(coord.x+i, coord.y+k);
			if (!((i==0)&&(k==0))
					&& (neighbourCoord.x < imageDim.x)
					&& (neighbourCoord.y < imageDim.y)
					&& (neighbourCoord.x > 0)
					&& (neighbourCoord.y > 0)){

				if ((getState(neighbourCoord, image)).x == 1.0f)
					counter++;
			}
		}
	}

	return counter;
}

__kernel void nextGeneration(__read_only image2d_t imageA, __write_only image2d_t imageB) {
	int n;
	float4 state;
	float4 alive = (float4)(1.0f, 0.0f, 0.0f, 1.0f);
	float4 dead = (float4)(0.0f, 0.0f, 0.0f, 1.0f);

	int2 coord = (int2)(get_global_id(0), get_global_id(1));
	int2 imageDim = get_image_dim(imageA);

	if ((coord.x<imageDim.x) && (coord.y<imageDim.y) && (coord.x>0) && (coord.y>0)) {
		n = getNumberOfNeighbours(coord, imageDim, imageA);
		state = getState(coord, imageA);

		if (state.x == alive.x) {
			if ((n>3) || (n<2))
				setState(coord, dead, imageB);
			else
				setState(coord, alive, imageB);
		} else if (state.x == dead.x) {
			if (n==3)
				setState(coord, alive, imageB);
			else
				setState(coord, dead, imageB);
		}
	}
}
