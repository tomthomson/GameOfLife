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
#define POPULATION 0.4

/**
* Width and height of the Game of Life board
*/
#define WIDTH 512
#define HEIGHT 512

/**
* Macro for OpenGL buffer offset
*/
#define BUFFER_DATA(i) ((char *)0 + i)
/********************************************
*            End of definitions
*********************************************/

using namespace std;

/* Create an instance of GameOfLife */
GameOfLife GameOfLife(RULES, POPULATION, WIDTH, HEIGHT);

/* Global variables for OpenGL */
GLuint glPBO, glTex, glShader;
void *font = GLUT_BITMAP_8_BY_13;
bool mouseLeftDown;
bool mouseRightDown;
float mouseX, mouseY;
float cameraDistance;
float verticalMove;
float horizontalMove;
float clampZoom = 1.0f;
float clampMove = 0.0f;
bool grid = false;
GLfloat gridControlPoints[2][2][3] = {
	{{-1.0, -1.0, 0.0}, {1.0, -1.0, 0.0}},
	{{-1.0, 1.0, 0.0}, {1.0, 1.0, 0.0}}
};
static const char *shaderCode =		/* glShader for displaying floating-point texture */
			"!!ARBfp1.0\n"
			"TEX result.color, fragment.texcoord, texture[0], 2D; \n"
			"END";

/* Show parameter help */
void showHelp() {
	cout << "Usage: GameOfLife [OPTION]... \n"
		<< "\t-cl <0|1>\t implementation mode:\n"
		<< "\t\t\t 0 = use CPU, 1 = use OpenCL (default)\n"
		<< endl;
}

/* Read commandline arguments */
/*
bool readFlags(int argc, char *argv[]) {
	if (argc == 3) {
		if (strcmp(argv[1], "-cl") == 0) {
			switch (argv[2][0]) {
			case '0':
				GameOfLife.setOpenCL(false);
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
*/

/* Show keyboard controls for OpenGL window and Game of Life */
void showControls() {
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
	
	printf("Controls:\n");
	printf(" key  | state | description\n");
	printf("------|-------|------------\n");
	printf("space | %s | start/stop calculation of next generation\n",
					GameOfLife.isPaused() ? "stop " : "start");
	printf("  g   | %s | draw grid for board\n",
					grid ? " on  " : " off ");
	printf("  s   | %s | stop after calculation of every generation\n",
					GameOfLife.isSingleGeneration() ? " yes " : " no  ");
	printf("  a   | %s | read image of next generation asynchronously \n",
					GameOfLife.isReadSync() ? "sync " : "async");
	printf("  c   | %s | calculate next generation with CPU/OpenCL \n",
					GameOfLife.isCPUMode() ? " CPU " : " CL  ");
	printf("q/esc |       | quit\n\n");
}

/* Free host/device memory */
void freeMem(void) {
	GameOfLife.freeMem();
	
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	if (glPBO)
		glDeleteBuffers(1, &glPBO);
	if (glTex)
		glDeleteTextures(1, &glTex);
	if (glShader)
		glDeleteProgramsARB(1, &glShader);
}

/*
 * GLUT callback functions
 */
/* Display function */
void display() {
	/*
	 * Calculate next generation if game is not paused
	 */
	if(!GameOfLife.isPaused()) {
		GLubyte* bufferImage =
				(GLubyte *)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);

		if (bufferImage) {
		  	/* Write image of next generation directly on the mapped buffer */
			int state = GameOfLife.nextGeneration(bufferImage);
			
			/* Release the mapped buffer */
			glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
			
			if (state != 0)
				exit(0);
		}
	}
	
	/*
	 * Draw GameOfLife image/board
	 */
	/* Clear buffer */
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	
	/* Save the initial ModelView matrix before modifying ModelView matrix */
    glPushMatrix();
	glLoadIdentity();								// reset modelview matrix
	
	/* Transform camera */
	glScalef(cameraDistance,cameraDistance,1);		// zoom
	glTranslatef(verticalMove,horizontalMove,0.0f);	// move
	
	/* Load texture from PBO */
	glBindTexture(GL_TEXTURE_2D, glTex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT,
					GL_RGBA, GL_UNSIGNED_BYTE, BUFFER_DATA(0));

	/* Fragment program is required to display floating point texture */
	glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, glShader);
	glEnable(GL_FRAGMENT_PROGRAM_ARB);
	glDisable(GL_DEPTH_TEST);
	
	/* Draw textured geometry */
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, -1.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, 1.0f);
	glEnd();
	
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_FRAGMENT_PROGRAM_ARB);
	
	/* Draw grid */
	if (grid) {
		glColor3f(0.0f, 0.2f, 1.0f);
		glEnable(GL_MAP2_VERTEX_3);
		glMap2f(GL_MAP2_VERTEX_3,
			0.0, 1.0,	/* U ranges 0..1 */
			3,			/* U stride, 3 floats per coord */
			2,			/* U is 2nd order, ie. linear */
			0.0, 1.0,	/* V ranges 0..1 */
			2 * 3,		/* V stride, row is 2 coords, 3 floats per coord */
			2,			/* V is 2nd order, ie linear */
			&gridControlPoints[0][0][0]);	/* control points */
			
		glMapGrid2f(HEIGHT, 0.0, 1.0,WIDTH, 0.0, 1.0);
		
		glEvalMesh2(GL_LINE,
			0, HEIGHT,	/* Starting at 0 mesh HEIGHT steps. */
			0, WIDTH);	/* Starting at 0 mesh WIDTH steps. */
	}
	
	glPopMatrix();
	
	glutSwapBuffers();
	glutReportErrors();

	/* Append execution time of one generation to window title */
	char title[64];
	char mode[10];
	GameOfLife.getExecutionMode(mode);
	sprintf(title, "Game of Life @ %s @ %f ms/gen @ %i gens/copy @ generation %u",
					mode,
					GameOfLife.getExecutionTime(),
					GameOfLife.getGenerationsPerCopyEvent(),
					GameOfLife.getGenerations()
					);
	glutSetWindowTitle(title);
}

/* Idle function */
void idle(void) {
	glutPostRedisplay();
}

/* Keyboard function */
void keyboard(unsigned char key, int mouseX, int mouseY) {
	switch(key) {
		/* Pressing c switches CPU mode on/off */
		case 'c':
			GameOfLife.switchCPUMode();
			showControls();
			break;
		/* Pressing space starts/stops calculation of next generation */
		case ' ':
			GameOfLife.switchPause();
			showControls();
			break;
		/* Pressing s switches single generation mode on/off */
		case 's':
			GameOfLife.switchSingleGeneration();
			showControls();
			break;
		/* Pressing g switches grid for Game of Life board on/off */
		case 'g':
			grid = !grid;
			showControls();
			break;
		/* Pressing a switches synchronous reading of images on/off */
		case 'a':
			GameOfLife.switchreadSync();
			showControls();
			break;
		/* Pressing escape or q exits */
		case 27:
		case 'q':
		case 'Q':
			exit(0);
		default:
			break;
	}
}

/* Reshape function */
void reshape(int w, int h) {
	glViewport(0, 0, w, h);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, 1.0, 0.0, 1.0, 0.0, 1.0);
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
	/* Move board */
	if(mouseLeftDown) {
		verticalMove = min(clampMove, max(-clampMove, verticalMove - (x - mouseX) * 0.01f));
		horizontalMove = min(clampMove, max(-clampMove, horizontalMove + (y - mouseY) * 0.01f));
	}
	/* Zoom board */
	if(mouseRightDown) {
		cameraDistance = max(clampZoom, cameraDistance - (y - mouseY) * 0.05f);
		clampMove = (cameraDistance-1) / cameraDistance;
		verticalMove = min(clampMove, max(-clampMove, verticalMove - (x - mouseX) * 0.01f));
		horizontalMove = min(clampMove, max(-clampMove, horizontalMove + (y - mouseY) * 0.01f));
	}
	mouseX = x;
	mouseY = y;
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
	glutInitWindowSize(WIDTH, HEIGHT);		// window size
	glutInitWindowPosition(400, 100);		// window location
	
	/* Create a window with OpenGL context */
	int handle = glutCreateWindow("Conway's Game of Life with OpenCL");
	
	/* Check available extensions */
	char *extensions = (char *)glGetString(GL_EXTENSIONS);
	if (NULL == strstr(extensions, "GL_ARB_vertex_buffer_object")) {
		cerr << "Video card does NOT support OpenGL"
				<< "extension GL_ARB_vertex_buffer_object." << endl;
		exit(-1);
	}
	if (NULL == strstr(extensions, "GL_ARB_pixel_buffer_object")) {
		cerr << "Video card does NOT support OpenGL"
				<< "extension GL_ARB_pixel_buffer_object." << endl;
		exit(-1);
	}
	
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
	glEnable(GL_TEXTURE_2D);
	
	glClearColor(36./255., 36./255., 85./255., 1.0);	// background color
	glClearStencil(0);					// clear stencil buffer
	glClearDepth(1.0f);					// depth buffer setup: 0 is near, 1 is far
	glDepthFunc(GL_LEQUAL);				// type of depth test
}

GLuint compileASMShader(GLenum program_type, const char *code) {
	GLuint programID;
	glGenProgramsARB(1, &programID);
	glBindProgramARB(program_type, programID);
	glProgramStringARB(program_type, GL_PROGRAM_FORMAT_ASCII_ARB,
						(GLsizei) strlen(code), (GLubyte *) code);
	
	GLint error_pos;
	glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &error_pos);
	if (error_pos != -1) {
		const GLubyte *error_string;
		error_string = glGetString(GL_PROGRAM_ERROR_STRING_ARB);
		cerr << "OpenGL program error at position: " << (int)error_pos << endl;
		cerr << error_string << endl;
		return 0;
	}
	return programID;
}

/* Initialise OpenGL Pixel Buffer Objects */
void initOpenGLBuffers() {
	glGenTextures(1, &glTex);
	glBindTexture(GL_TEXTURE_2D, glTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WIDTH, HEIGHT,
					0, GL_RGBA, GL_UNSIGNED_BYTE, GameOfLife.getImage());
	
	/* Generate a new buffer object */
	glGenBuffers(1, &glPBO);
	/* Bind the buffer object */
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, glPBO);
	/* Copy pixel data to the buffer object */
	glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, WIDTH * HEIGHT * 4,
					GameOfLife.getImage(), GL_STREAM_DRAW_ARB);
	
	/* Load shader program */
	glShader = compileASMShader(GL_FRAGMENT_PROGRAM_ARB, shaderCode);
}

/* Initalise display */
void initDisplay(int argc, char *argv[]) {
	mouseLeftDown = mouseRightDown = false;
	cameraDistance = 1.0f;
	horizontalMove = 0.0f;
	verticalMove = 0.0f;
	
	initGLUT(argc, argv);
	initOpenGL();
	initOpenGLBuffers();
}

/* OpenGL main loop */
void mainLoopGL(void) {
	/* last GLUT call (LOOP)
	 * window will be shown and display callback is triggered by events
	 * NOTE: this call never returns to main() resp. mainLoopGL()
	 */
	glutMainLoop();
}

int main(int argc, char **argv) {
	
	/* Register exit function */
	atexit(freeMem);
	
	/* Read command line arguments */
	/*
	if (!readFlags(argc, argv)) {
		showHelp();
		return -1;
	}
	*/

	/* Setup host/device memory, starting population and OpenCL */
	if(GameOfLife.setup()!=0)
		return -1;
	
	/* Show controls for Game of Life in console */
	showControls();
	
	/* Setup OpenGL */
	initDisplay(argc, argv);
	
	/* Display GameOfLife image/board and calculate next generations */
	mainLoopGL();
	
	/* Calculate one generation for OpenCL profiler without OpenGL output */
	GameOfLife.nextGeneration(GameOfLife.getImage());
	cout << GameOfLife.getExecutionTime() << endl;
	
	return 0;
}
