/**
 * Name:        GameOfLife
 * Author:      Thomas Rumpf
 * Description: John Conway's Game of Life with OpenCL
 */

/********************************************
*          Start of definitions
*********************************************/
#define GL_GLEXT_PROTOTYPES

#if defined(__APPLE__) || defined(MACOSX)
	#include <GLUT/glut.h>
#else
	#include <GL/glut.h>
#endif

#include "GL/glext.h"
#include <string.h>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>

#include "../inc/GameOfLife.hpp"

/** 
* Rules for calculating next generation
* 0:	Standard rules by John Conway
  ---------------------------------
	Dead cell:
		Becomes live cell, if it has exactly three live neighbours.
		Stays dead, if it has not three live neighbours.
	Live cell:
		Survives, if it has two or three live neighbours.
		Dies, if it has one, two or more than thrre live neighbours.
	
* 1:	Custom rule
  -----------------
	Dead cell:
	Live cell:
*/
/* Rule definitions: nextState = rule[state * numberOfNeighbours] */
#define RULES 0

/** 
* Chance to create a new individual
* when using random starting population
*/
#define POPULATION 0.04

/**
* Width and height of the Game of Life board
*/
#define WIDTH 512
#define HEIGHT 512
/********************************************
*            End of definitions
*********************************************/

using namespace std;

/* Create an instance of GameOfLife */
GameOfLife GameOfLife(RULES, POPULATION, WIDTH, HEIGHT);

/* Global variables for OpenGL */
static unsigned char *globalImage;
void *font = GLUT_BITMAP_8_BY_13;
bool mouseLeftDown;
bool mouseRightDown;
float mouseX, mouseY;
float cameraAngleX;
float cameraAngleY;
float cameraDistance;

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

/* Show keyboard controls for OpenGL window and Game of Life */
void showControls() {
	cout << "Controls:\n";
	cout << "space \t start/stop calculation of next generation\n";
	cout << "  g   \t switch single generation mode on/off\n";
	cout << "q/esc \t quit\n" << endl;
}

/* Free host/device memory */
void freeMem(void) {
	GameOfLife.freeMem();
}

/*
 * GLUT support functions
 */
/* Write 2d text using GLUT
 * The projection matrix must be set to orthogonal before calling this function. */
void drawString(const char *str, int x, int y, float color[4], void *font) {
	glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT); // lighting and color mask
	glDisable(GL_LIGHTING);		// need to disable lighting for proper text color

	glColor4fv(color);			// set text color
	glRasterPos2i(x, y);		// place text position

	// loop all characters in the string
	while(*str) {
		glutBitmapCharacter(font, *str);
		++str;
	}

	glEnable(GL_LIGHTING);
	glPopAttrib();
}

/* Show infos about calculations */
void showInfo() {
	/* Append execution time of one generation to window title */
	char title[64];
	sprintf(title, "Conway's Game of Life with OpenCL @ %f ms/generation",
					GameOfLife.getExecutionTime());
	glutSetWindowTitle(title);
	
	
	// backup current model-view matrix
	glPushMatrix();						// save current modelview matrix
	glLoadIdentity();					// reset modelview matrix

	// set to 2D orthogonal projection
	glMatrixMode(GL_PROJECTION);		// switch to projection matrix
	glPushMatrix();						// save current projection matrix
	glLoadIdentity();					// reset projection matrix
	gluOrtho2D(0, 400, 0, 300);			// set to orthogonal projection

	float color[4] = {1, 1, 1, 1};

	stringstream ss;
	/*
	ss << "VBO: " << (vboUsed ? "on" : "off") << std::ends;  // add 0(ends) at the end
	drawString(ss.str().c_str(), 1, 286, color, font);
	ss.str(""); // clear buffer
	*/
	/*
	ss << std::fixed << std::setprecision(3);
	ss << "Updating Time: " << updateTime << " ms" << std::ends;
	drawString(ss.str().c_str(), 1, 272, color, font);
	ss.str("");

	ss << "Drawing Time: " << drawTime << " ms" << std::ends;
	drawString(ss.str().c_str(), 1, 258, color, font);
	ss.str("");
	*/
	ss << "Press SPACE key to toggle VBO on/off." << std::ends;
	drawString(ss.str().c_str(), 1, 280, color, font);

	// unset floating format
	ss << std::resetiosflags(std::ios_base::fixed | std::ios_base::floatfield);

	// restore projection matrix
	glPopMatrix();                   // restore to previous projection matrix

	// restore modelview matrix
	glMatrixMode(GL_MODELVIEW);      // switch to modelview matrix
	glPopMatrix();                   // restore to previous modelview matrix
}
/*
 * GLUT callback functions
 */
/* Display function */
void display() {
	
	/* Clear buffer */
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	
	/* Save the initial ModelView matrix before modifying ModelView matrix */
    glPushMatrix();
	
	/* Transform camera */	
    glTranslatef(0, 0, cameraDistance);
    glRotatef(cameraAngleX, 1, 0, 0);	// pitch
    glRotatef(cameraAngleY, 0, 1, 0);	// heading
	
	/* Draw GameOfLife image/board */
	glDrawPixels(WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, globalImage);
	
	/* Draw infos */
	showInfo();
	
	glPopMatrix();
	
	glutSwapBuffers();
	
	/* Calculate next generation if game not paused */
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
		/* Pressing space starts/stops calculation of next generation */
		case 'g':
			GameOfLife.singleGeneration();
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

/* Reshape function */
void reshape(int width, int height) {

}

/* Mouse function */
void mouse(int button, int state, int x, int y) {
	mouseX = x;
	mouseY = y;

	if(button == GLUT_LEFT_BUTTON) {
		
		if(state == GLUT_DOWN) {
			mouseLeftDown = true;
		} else if(state == GLUT_UP)
			mouseLeftDown = false;
		
	} else if(button == GLUT_RIGHT_BUTTON) {
		
		if(state == GLUT_DOWN) {
			mouseRightDown = true;
		} else if(state == GLUT_UP)
			mouseRightDown = false;
		
	}
}

/* Mouse motion function */
void mouseMotion(int x, int y) {
	if(mouseLeftDown) {
		cameraAngleY += (x - mouseX);
		cameraAngleX += (y - mouseY);
		mouseX = x;
		mouseY = y;
	}
	if(mouseRightDown) {
		cameraDistance += (y - mouseY) * 0.2f;
		mouseY = y;
	}
}
/*
 * End of GLUT callback functions
 */

/* Initialise OpenGL Utility Toolkit for windowing */
int initGLUT(int argc, char *argv[]) {
	
	/* Initialise window */
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE |		// double buffer
						GLUT_DEPTH  |		// depth buffer available
						GLUT_RGBA   |		// color buffer with reg, green, blue, alpha
						GLUT_ALPHA);
	glutInitWindowSize(WIDTH, HEIGHT+50);	// window size
	glutInitWindowPosition(200, 100);		// window location
	
	/* Create a window with OpenGL context */
	int handle = glutCreateWindow("Conway's Game of Life with OpenCL");
	
	/* Register GLUT callback functions */
	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutKeyboardFunc(keyboard);
	glutReshapeFunc(reshape);
	glutMouseFunc(mouse);
	glutMotionFunc(mouseMotion);
	
	return handle;
}

/* Initalise OpenGL */
void initOpenGL(void) {
	/* Shading method: GL_SMOOTH or GL_FLAT */
	glShadeModel(GL_SMOOTH);
	/* 4-byte pixel alignment */
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	
	/* Hints */
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	
	/* Enable features */
	glEnable(GL_DEPTH_TEST);			// enables depth testing
	
	glClearColor(0.0, 0.0, 0.0, 1.0);	// background color
	glClearStencil(0);					// clear stencil buffer
	glClearDepth(1.0f);					// depth buffer setup: 0 is near, 1 is far
	glDepthFunc(GL_LEQUAL);				// type of depth test
}

/* Initalise display */
void initDisplay(int argc, char *argv[]) {
	mouseLeftDown = mouseRightDown = false;
	initGLUT(argc, argv);
	initOpenGL();
}

/* OpenGL main loop */
void mainLoopGL(void) {
	glutMainLoop();
}

int main(int argc, char **argv) {
	
	//printf("\033[2J");
	#ifdef WIN32
		system("cls");
	#else
		system("clear");
	#endif
	cout << "----------------------------------" << endl;
	cout << "---- John Conway's            ----" << endl;
	cout << "---- Game of Life with OpenCL ----" << endl;
	cout << "---- by Thomas Rumpf          ----" << endl;
	cout << "----------------------------------" << endl;
	
	/* read command line arguments */
	if (!readFlags(argc, argv)) {
		showHelp();
		return -1;
	}

	/* Setup host/device memory, starting population and OpenCL */
	if(GameOfLife.setup()!=0)
		return -1;

	/* Allocate and update image for OpenGL display */
	globalImage = (unsigned char*) malloc (GameOfLife.imageSizeBytes);
	memcpy(globalImage, GameOfLife.image, GameOfLife.imageSizeBytes);
	
	/* Display GameOfLife image/board and calculate next generations */
	initDisplay(argc, argv);
	showControls();
	mainLoopGL();
	
	//GameOfLife.nextGeneration();

	/* Free host/device memory */
	freeMem();
	return EXIT_SUCCESS;
}
