/**
 * Name:        GameOfLife
 * Author:      Thomas Rumpf
 * Description: John Conway's Game of Life with OpenCL
 */

/********************************************
*                Definitions
*********************************************/
#define GL_GLEXT_PROTOTYPES

#if defined(__APPLE__) || defined(MACOSX)
	#include <GLUT/glut.h>
#else
	#include <GL/glut.h>
#endif

#include "GL/glext.h"
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include "anyoption.h"	/* for command line parsing */

#include "../inc/GameOfLife.hpp"

/**
* Macro for OpenGL buffer offset
*/
#define BUFFER_DATA(i) ((char *)0 + i)

using namespace std;

/********************************************
*            Global variables
*********************************************/
/* Create an instance of GameOfLife */
GameOfLife GameOfLife;

/* Global variables for OpenGL */
GLuint glPBO, glTex, glShader;
int GLUTWindowHandle;
bool mouseLeftDown, mouseRightDown;
float mouseX, mouseY;
float cameraDistance;
float verticalMove, horizontalMove;
float clampZoom = 1.0f;
float clampMove = 0.0f;
bool drawGrid = false;
GLfloat gridControlPoints[2][2][3] = {
		{{-1.0, -1.0, 0.0}, {1.0, -1.0, 0.0}},
		{{-1.0, 1.0, 0.0}, {1.0, 1.0, 0.0}}
	};
static const char *shaderCode =		/* glShader for displaying floating-point texture */
			"!!ARBfp1.0\n"
			"TEX result.color, fragment.texcoord, texture[0], 2D; \n"
			"END";

/********************************************
*            Global functions
*********************************************/
/* Setup command line parser */
void setupCommandlineParser(AnyOption *opt) {
	/* Set usage/help */
	opt->addUsage( "Usage: GameOfLife -f PATH [-l RULE] WIDTH [HEIGHT] " );
	opt->addUsage( "  or:  GameOfLife -r DENSITY [-l RULE] WIDTH [HEIGHT]" );
	opt->addUsage( "" );
	opt->addUsage( "Mandatory arguments to long options are mandatory for short options too.");
	opt->addUsage( " -h  --help  	       Prints this help " );
	opt->addUsage( " -f  --file PATH       Path to filename used for starting population");
	opt->addUsage( " -r  --random DENSITY  Use random starting population with given density");
	opt->addUsage( " -l  --rule RULE       rule for next generations as a list of Survival/Birth");
	opt->addUsage( "                       default: 23/3");
	opt->addUsage( "                       defintion is overwritten when there is a");
	opt->addUsage( "                       rule specified in the file");
	opt->addUsage( "" );

	/* Set the option string/characters */
	/* by default all options will be checked on the command line */
	opt->setCommandFlag("help",'h');
	opt->setCommandOption("file",'f');
	opt->setCommandOption("random",'r');
	opt->setCommandOption("rule",'l');
}

/* Read commandline arguments */
int readArguments(AnyOption *opt, int argc, char *argv[]) {
	/* Go through the command line and get the options */
    opt->processCommandArgs(argc,argv);
	
	/* Print usage if help needed */
	if (opt->getFlag("help") || opt->getFlag('h')) {
		return -1;
	}
	
	/* Get file or random population */
	if (opt->getValue("file") != NULL || opt->getValue('f') != NULL) {
		GameOfLife.setFilename(opt->getValue('f'));
	} else if (atof(opt->getValue("random")) != 0.0f || atof(opt->getValue('r')) != 0.0f) {
		GameOfLife.setPopulation(atof(opt->getValue('r')));
	} else {
		return -1;
	}
	
	/* Get rule */
	int rule = 0;
	if (opt->getValue("rule") != NULL || opt->getValue('l') != NULL) {
		if (GameOfLife.setRule(opt->getValue('l')) != 0) {
			cerr << "Error in rule definition" << endl;
			return -1;
		}
		rule = 2;
	} else {
		char defaultRule[] = "23/3";
		GameOfLife.setRule(defaultRule);
	}
	
	/* Get width and height */
	if ((unsigned int)argc == 4+rule)
		GameOfLife.setSize(atoi(argv[3+rule]),atoi(argv[3+rule]));
	else if ((unsigned int)argc == 5+rule)
		GameOfLife.setSize(atoi(argv[3+rule]),atoi(argv[3+rule]));
	else
		return -1;
	
	delete opt;
	return 0;
}

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
	printf("Preferences:\n");
	printf("rule: %s | mode: %s | width: %i | height: %i \n",
			GameOfLife.getRule().c_str(),
			GameOfLife.isFileMode() ? "file" : "random",
			GameOfLife.getWidth(), GameOfLife.getHeight());
	printf("\n");
	printf("Controls:\n");
	printf(" key  | state | description\n");
	printf("------|-------|------------\n");
	printf("space | %s | start/stop calculation of next generation\n",
					GameOfLife.isPaused() ? "stop " : "start");
	printf("  g   | %s | draw grid for board\n",
					drawGrid ? " on  " : " off ");
	printf("  s   | %s | stop after calculation of every generation\n",
					GameOfLife.isSingleGeneration() ? " yes " : " no  ");
	printf("  a   | %s | read image of next generation asynchronously \n",
					GameOfLife.isReadSync() ? "sync " : "async");
	printf("  c   | %s | calculate next generation with CPU/OpenCL \n",
					GameOfLife.isCPUMode() ? " CPU " : " CL  ");
	printf("  r   |       | reset to starting population \n");
	printf("q/esc |       | quit\n\n");
}

/* Free host/device memory */
void freeMem(void) {
	GameOfLife.freeMem();
	
	if (glPBO) {
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, glPBO);
		glDeleteBuffers(1, &glPBO);
		glPBO = 0;
	}
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	if (glTex) {
		glDeleteTextures(1, &glTex);
		glTex = 0;
	}
	if (glShader) {
		glDeleteProgramsARB(1, &glShader);
	}
	if (GLUTWindowHandle)
		glutDestroyWindow(GLUTWindowHandle);
}

/********************************************
*         GLUT callback functions
*********************************************/
/* Display function */
void display() {
	/*
	 * Calculate next generation if game is not paused
	 */
	if(!GameOfLife.isPaused()) {
		/*
		 * Map buffer to host memory space
		 * and return address of buffer in host address space
		 */
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
	
	/* Download texture from PBO */
	glBindTexture(GL_TEXTURE_2D, glTex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GameOfLife.getWidth(), GameOfLife.getHeight(),
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
	if (drawGrid) {
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
			
		glMapGrid2f(GameOfLife.getWidth(), 0.0, 1.0, GameOfLife.getHeight(), 0.0, 1.0);
		
		glEvalMesh2(GL_LINE,
			0, GameOfLife.getWidth(),	/* Starting at 0 mesh GameOfLife.getWidth() steps. */
			0, GameOfLife.getHeight());	/* Starting at 0 mesh GameOfLife.getHeight() steps. */
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
			drawGrid = !drawGrid;
			showControls();
			break;
		/* Pressing a switches synchronous reading of images on/off */
		case 'a':
			GameOfLife.switchreadSync();
			showControls();
			break;
		/* Pressing r resets the board to the starting population */
		case 'r':
			GameOfLife.reset();
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
		//verticalMove = min(clampMove, max(-clampMove, verticalMove - (x - mouseX) * 0.01f));
		//horizontalMove = min(clampMove, max(-clampMove, horizontalMove + (y - mouseY) * 0.01f));
	}
	mouseX = x;
	mouseY = y;
}

/********************************************
*         GLUT init functions
*********************************************/ 
/* Initialise OpenGL Utility Toolkit for windowing */
int initGLUT(int argc, char *argv[]) {
	
	/* Initialise window */
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE |		// double buffer
						GLUT_DEPTH  |		// depth buffer available
						GLUT_RGBA   |		// color buffer with reg, green, blue, alpha
						GLUT_ALPHA);
	int windowHeight = glutGet(GLUT_SCREEN_HEIGHT)/2;
	int windowWidth = windowHeight * (GameOfLife.getWidth()/
	                                  GameOfLife.getHeight());
	// window size
	glutInitWindowSize(windowWidth, windowHeight);
	// window location
	glutInitWindowPosition(glutGet(GLUT_SCREEN_WIDTH)-windowWidth-50,
	                       glutGet(GLUT_SCREEN_HEIGHT)-windowHeight-50);
	
	/* Create a window with OpenGL context */
	GLUTWindowHandle = glutCreateWindow("Conway's Game of Life with OpenCL");
	
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
	
	return 0;
}

/* Initalise OpenGL */
void initOpenGL(void) {
	/* Shading method: GL_SMOOTH or GL_FLAT */
	glShadeModel(GL_SMOOTH);
	
	//glPixelStorei(GL_UNPACK_ROW_LENGTH, GameOfLife.getWidth());
	
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, GameOfLife.getWidth(), GameOfLife.getHeight(),
					0, GL_RGBA, GL_UNSIGNED_BYTE, GameOfLife.getImage());
	
	/* Generate a new buffer object */
	glGenBuffers(1, &glPBO);
	/* Bind the buffer object */
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, glPBO);
	/* Copy pixel data to the buffer object */
	glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, GameOfLife.getWidth() * GameOfLife.getHeight() * 4,
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

/********************************************
*             MAIN functions
*********************************************/
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
	
	/* Create an instance of command line parser */
	AnyOption *opt = new AnyOption();
	
	/* Setup command line parser */
	setupCommandlineParser(opt);
	
	/* Read command line arguments */
	if (readArguments(opt, argc, argv) != 0) {
		opt->printUsage();
	    delete opt;
		return -1;
	}

	/* Setup host/device memory, starting population and OpenCL */
	if(GameOfLife.setup()!=0) return -1;
	
	/* Show controls for Game of Life in console */
	showControls();
	
	/* Setup OpenGL */
	initDisplay(argc, argv);
	
	/* Display GameOfLife image/board and calculate next generations */
	mainLoopGL();
	
	/* Calculate one generation for OpenCL profiler without OpenGL output */
	//GameOfLife.nextGeneration(GameOfLife.getImage());
	//cout << GameOfLife.getExecutionTime() << endl;
	
	return 0;
}
