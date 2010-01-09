int getState(int x, int y, int width, __global int *image) {
	return (image[x + (width*y)]);
}

void setState(int x, int y, int state, int width, __global int *image) {
	image[x + (width*y)] = state;
}

int getNumberOfNeighbours(const int x, const int y, const int width, const int height, __global int *image) {
	
	int counter = 0;

	for (int i=-1; i<=1; i++) {
		for (int k=-1; k<=1; k++) {
			if (!((i==0)&&(k==0)) && (x+i<width) && (y+k<height) && (x+i>0) && (y+k>0)) {
				if (getState(x+i, y+k, width, image) > 0)
					counter++;
			}
		}
	}
	return counter;
	
}

__kernel void nextGeneration(__global int *imageA, __global int *imageB, const int width, const int height) {
	int state, n;

	int x = get_global_id(0);
	int y = get_global_id(1);

	if ((x<width) && (y<height) && (x>0) && (y>0)) {
		n = getNumberOfNeighbours(x, y, width, height, imageA);
		
		state = getState(x, y, width, imageA);

		if (state>0) {
			if ((n>3) || (n<2))
				setState(x, y, 0, width, imageB);
			else
				setState(x, y, state==255?255:state+1, width, imageB);
		} else if (state==0) {
			if (n==3)
				setState(x, y, 1, width, imageB);
			else
				setState(x, y, 0, width, imageB);
		}

	}
}
