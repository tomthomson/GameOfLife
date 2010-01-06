#ifdef linux
#define GL_GLEXT_PROTOTYPES
#endif
#include <GL/glew.h>
#include "../inc/GameOfLifeMain.hpp"
#include "../inc/GameOfLife.hpp"
#if defined(__APPLE__) || defined(MACOSX)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <CL/cl_gl.h> // extra CL/GL include
#include <cstdlib>
#include <cstdio>

/* chance, that the random starting population generator decides to create a new individual */
#define POPULATION 0.3125

/* An instance of the GameOfLife Class */
GameOfLife GameOfLife(POPULATION);

/* window height, window Width and the pixels to be displayed */
int width;
int height;
unsigned char* pixels;

/* display function */
void displayFunc() {
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glFlush();
	glutSwapBuffers();
}

/* idle function */
void idleFunc(void) {
    glutPostRedisplay();
}

/* keyboard function */
void keyboardFunc(unsigned char key, int mouseX, int mouseY) {
	switch(key) {
		/* If the user hits escape or Q, then exit */
		case 27:
		case 'q':
		case 'Q':
			cleanup();
			exit(0);
		default:
			break;
	}
}

/* initalise display */
void initDisplay(int argc, char *argv[]) {
	initGlut(argc, argv);
	initGL();
}

/* initalise glut */
void initGlut(int argc, char *argv[]) {
	/* initialising the window */
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowSize(width, height);
	glutInitWindowPosition(0,0);
	glutCreateWindow("Conway's Game of Life with OpenCL");
	glClearColor(0, 0, 0, 1.0);


	/* the various glut callbacks */
	glutDisplayFunc(displayFunc);
	glutIdleFunc(idleFunc);
	glutKeyboardFunc(keyboardFunc);
}

/*initalise OpenGL */
void initGL(void) {
	glewInit();
}

void mainLoopGL(void) {
	glutMainLoop();
}

/* free any allocated resources */
void cleanup(void) {
	if(pixels)
		free(pixels);
	GameOfLife.cleanup();
}

int main(int argc, char **argv) {
	/* setup host/device memory and OpenCL */
	if(GameOfLife.setup()!=0)
		return -1;
	/* run OpenCL GameOfLife */
	if(GameOfLife.run()!=0)
		return -1;

	/* display GameOfLife board */
	width = GameOfLife.getWidth();
	height = GameOfLife.getHeight();
	int* output = GameOfLife.getPixels();
	pixels = (unsigned char *)malloc(height*width*4*sizeof(unsigned char));
	for(int i=0; i< width*height; ++i) {
		pixels[4*i]     = (unsigned char)output[i]*(i/width)/height;
		pixels[4*i + 1] = (unsigned char)output[i]*(i%width)/width ;
		pixels[4*i + 2] = (unsigned char)output[i]*i/(width*height);
		pixels[4*i + 3] = 255;
	}

	initDisplay(argc, argv);
	mainLoopGL();

	cleanup();
	return EXIT_SUCCESS;
}
