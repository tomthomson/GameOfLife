#ifdef linux
#define GL_GLEXT_PROTOTYPES
#endif
#include <GL/glew.h>
#if defined(__APPLE__) || defined(MACOSX)
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glut.h>
#endif
//#include <CL/cl_gl.h> // CL/GL interoperation
#include <cstdlib>
#include <cstdio>

#include "../inc/GameOfLifeMain.hpp"
#include "../inc/GameOfLife.hpp"

/* Create an instance of the GameOfLife class */
GameOfLife GameOfLife(POPULATION, WIDTH, HEIGHT);

/* Free host/device memory */
void freeMem(void) {
	GameOfLife.freeMem();
}

/* Initalise display */
void initDisplay(int argc, char *argv[]) {
	initGlut(argc, argv);
	initGL();
}

/* Display function */
void displayFunc() {
	glMatrixMode(GL_PROJECTION);    /* specifies the current matrix */
	glLoadIdentity();               /* Sets the currant matrix to identity */
	gluOrtho2D(0,WIDTH,0,HEIGHT);   /* Sets the clipping rectangle extends */

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glEnable(GL_BLEND);             /* enable blending */
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glColor3f(1,1,1);
	glPointSize (1);

	int state;
	glBegin(GL_POINTS);
	for (int x=1; x<WIDTH; x++) {
		for (int y=1; y<HEIGHT; y++) {
			state = GameOfLife.getState(x,y);
			if (state>0) {
				glColor3f((float)state/255.,1.-((float)state/255.),0);
				glVertex3f(x,y,0);
			}
		}
	}
	glEnd();

	glutSwapBuffers();
	glutPostRedisplay();
	glFlush();

	if(GameOfLife.nextGeneration()!=0) {
		freeMem();
		exit(0);
	}
}

/* Idle function */
void idleFunc(void) {
	glutPostRedisplay();
}

/* Keyboard function */
void keyboardFunc(unsigned char key, int mouseX, int mouseY) {
	switch(key) {
		/* If the user hits escape or Q, then exit */
		case 27:
		case 'q':
		case 'Q':
			freeMem();
			exit(0);
		default:
			break;
	}
}

/* Initialise GLUT */
void initGlut(int argc, char *argv[]) {
	/* Initialise window */
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_ALPHA | GLUT_DEPTH);
	glutInitWindowSize(WIDTH, HEIGHT);
	glutInitWindowPosition(100,100);
	glutCreateWindow("Conway's Game of Life with OpenCL");
	glClearColor(0, 0, 0, 1.0);

	/* Initialise callbacks */
	glutDisplayFunc(displayFunc);
	glutIdleFunc(idleFunc);
	glutKeyboardFunc(keyboardFunc);
}

/* Initalise OpenGL */
void initGL(void) {
	glewInit();
}

/* OpenGL main loop */
void mainLoopGL(void) {
	glutMainLoop();
}

int main(int argc, char **argv) {
	/* Setup host/device memory and OpenCL */
	if(GameOfLife.setup()!=0)
		return -1;

	/* Display GameOfLife board and calculate next generations */
	initDisplay(argc, argv);
	mainLoopGL();

	/* Free host/device memory */
	freeMem();
	return EXIT_SUCCESS;
}
