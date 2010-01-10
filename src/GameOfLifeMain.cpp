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
#include <cstdio>
#include <cstdlib>
#include "../inc/GameOfLifeMain.hpp"
#include "../inc/GameOfLife.hpp"

/* Create an instance of GameOfLife */
GameOfLife GameOfLife(POPULATION, WIDTH, HEIGHT);

/* Free host/device memory */
void freeMem(void) {
	GameOfLife.freeMem();
}

/* Display function */
void display() {
	glMatrixMode(GL_PROJECTION);    /* specify that we want to modify the projection matrix */
	glLoadIdentity();               /* Sets the currant matrix to identity */
	gluOrtho2D(0,WIDTH,0,HEIGHT);   /* Sets the clipping rectangle extends */

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);     /* specify that we want to modify the modelview matrix */
	glLoadIdentity();
	glEnable(GL_BLEND);             /* enable blending */
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glColor3f(1,1,1);
	glPointSize(3);

	unsigned char state;            /* state of the current cell */
	glBegin(GL_POINTS);
	for (int x=1; x<WIDTH; x++) {
		for (int y=1; y<HEIGHT; y++) {
			state = GameOfLife.getState(x,y);
			if (state>0) {
				glColor3f(0, state, 0);
				glVertex3f(x,y,0);
			}
		}
	}
	glEnd();

	glutSwapBuffers();              /* copy back-buffer into the front-buffer */
	glutPostRedisplay();            /* marks the current window as needing to be redisplayed */
	glFlush();                      /* draw OpenGL commands */

	if(!GameOfLife.isPaused() && GameOfLife.nextGeneration()!=0) {
		freeMem();
		exit(0);
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
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_ALPHA | GLUT_DEPTH);
	glutInitWindowSize(WIDTH, HEIGHT);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Conway's Game of Life with OpenCL");
	glClearColor(0, 0, 0, 1.0);

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
