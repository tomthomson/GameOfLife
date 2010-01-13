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

using namespace std;

/* Create an instance of GameOfLife */
GameOfLife GameOfLife(POPULATION, WIDTH, HEIGHT);

/* Global variables */
unsigned char *globalImage;

/* Show parameter help */
void showHelp() {
	cout << "Usage: GameOfLife [OPTION]... \n"
		<< "\t-cl <0|1>\t implementation mode;\n\t\t\t 0 = use CPU, 1 = use OpenCL (default)\n"
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
	glMatrixMode(GL_PROJECTION);    /* specify that we want to modify the projection matrix */
	glLoadIdentity();               /* Sets the currant matrix to identity */
	gluOrtho2D(0,WIDTH,0,HEIGHT);   /* Sets the clipping rectangle extends */

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);     /* specify that we want to modify the modelview matrix */
	glLoadIdentity();
	glEnable(GL_BLEND);             /* enable blending */
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glPointSize(2);
	unsigned char state;            /* state of the current cell */

	glBegin(GL_POINTS);
	for (int x=1; x<WIDTH; x++) {
		for (int y=1; y<HEIGHT; y++) {
			state = GameOfLife.getState(x,y);
			if (state>0) {
				glColor3f(0, 0, state);
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

void display2() {
	globalImage = GameOfLife.image;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDrawPixels(WIDTH, HEIGHT, GL_LUMINANCE, GL_UNSIGNED_BYTE,globalImage);
	glutSwapBuffers();
	glFlush();

	if(!GameOfLife.isPaused() && GameOfLife.nextGenerationOpenCL()!=0) {
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
	glClearColor(0.0, 0.0, 0.0, 1.0);

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

	/* Display GameOfLife board and calculate next generations */
	initDisplay(argc, argv);
	showControls();
	mainLoopGL();

	/* Free host/device memory */
	freeMem();
	return EXIT_SUCCESS;
}
