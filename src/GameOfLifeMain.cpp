#ifdef linux
#define GL_GLEXT_PROTOTYPES
#endif
//#include <GL/glew.h>
#if defined(__APPLE__) || defined(MACOSX)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#include <GL/gl.h>
#endif

#include <string.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include "../inc/GameOfLifeMain.hpp"
#include "../inc/GameOfLife.hpp"

using namespace std;

/* Create an instance of GameOfLife */
GameOfLife GameOfLife(RULES, POPULATION, WIDTH, HEIGHT);

/* Global variables */
static unsigned char *globalImage;

/* Show parameter help */
void showHelp() {
	cout << "Usage: GameOfLife [OPTION]... \n"
		<< "\t-cl <0|1>\t implementation mode:\n"
		<< "\t\t\t 0 = use CPU, 1 = use OpenCL (default)\n"
		<< endl;
}

/* Read commandline arguments */
bool readFlags(int argc, char *argv[]) {
	if (argc == 3) {
		if (strcmp(argv[1], "-cl") == 0) {
			switch (argv[2][0]) {
			case '0':
				GameOfLife.useOpenCL = false;
				break;
			case '1':
				break;
			default:
				cout << "invalid implementation mode" << endl;
				return false;
			}
		} else {
			return false;
		}
	} else {
		if (argc != 1)
			return false;
	}
	return true;
}

void showControls() {
	std::cout << "Controls:\n";
	std::cout << "space \t start/stop calculation of next generation\n";
	std::cout << "q/esc \t quit\n" << endl;
}

/* Free host/device memory */
void freeMem(void) {
	GameOfLife.freeMem();
}

/* Display function */
void display() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDrawPixels(WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, globalImage);
	glutSwapBuffers();
	glutPostRedisplay();
	glFlush();
	if(!GameOfLife.isPaused()) {
		if (GameOfLife.nextGeneration()==0) {
			memcpy(globalImage, GameOfLife.image, GameOfLife.imageSizeBytes);
		} else {
			freeMem();
			exit(0);
		}
	}
	
}

/* Idle function */
void idle(void) {
	glutPostRedisplay();
}

/* Keyboard function */
void keyboard(unsigned char key, int mouseX, int mouseY) {
	switch(key) {
		/* Pressing space starts/stops calculation of next generation */
		case ' ':
			GameOfLife.pause();
			break;
			/* Pressing escape or q exits */
		case 27:
		case 'q':
		case 'Q':
			freeMem();
			exit(0);
		default:
			break;
	}
}

/* Initialise OpenGL Utility Toolkit */
void initGlut(int argc, char *argv[]) {
	/* Initialise window */
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE |	// double buffer
						GLUT_DEPTH  |	// depth buffer available
						GLUT_RGBA   |	// color buffer with reg, green, blue, alpha
						GLUT_ALPHA);
	glutInitWindowSize(WIDTH, HEIGHT);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Conway's Game of Life with OpenCL");
	glClearColor(0.0, 0.0, 0.0, 1.0);

	glClearDepth(1.0f);					// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);			// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);				// The Type Of Depth Test To Do

	/* Initialise callbacks */
	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutKeyboardFunc(keyboard);
}

/* Initalise OpenGL Extension Wrangler Library */
void initGlew(void) {
	//glewInit();
}

/* Initalise display */
void initDisplay(int argc, char *argv[]) {
	initGlut(argc, argv);
	initGlew();
}

/* OpenGL main loop */
void mainLoopGL(void) {
	glutMainLoop();
}

int main(int argc, char **argv) {
	/* read command line arguments */
	if (!readFlags(argc, argv)) {
		showHelp();
		return -1;
	}

	/* Setup host/device memory and OpenCL */
	if(GameOfLife.setup()!=0)
		return -1;

	/* Update global image for display2 */
	globalImage = (unsigned char*) malloc (GameOfLife.imageSizeBytes);
	memcpy(globalImage, GameOfLife.image, GameOfLife.imageSizeBytes);
	
	/* Display GameOfLife board and calculate next generations */
	initDisplay(argc, argv);
	showControls();
	mainLoopGL();
	
	//GameOfLife.nextGeneration();

	/* Free host/device memory */
	freeMem();
	return EXIT_SUCCESS;
}
