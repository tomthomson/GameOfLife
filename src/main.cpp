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
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <unistd.h>				/* for command line parsing */
#include <ctime>				/* for time() */
#include <cstdlib>				/* for srand() and rand() */
#ifdef WIN32				// Windows system specific
	#include <windows.h>		/* for QueryPerformanceCounter */
#else						// Unix based system specific
	#include <sys/time.h>		/* for gettimeofday() */
#endif

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
char title[57*(int)sizeof(char)+(int)sizeof(float)+(int)sizeof(int)+(int)sizeof(long)];
bool mouseLeftDown, mouseRightDown;
float mouseX, mouseY;
float cameraDistance;
float verticalMove, horizontalMove;
float clampZoom = 1.0f;
float clampMove = 0.0f;
bool drawGrid = false;
bool resetGame = false;
float sleeperBarrier = 0.0f;
#ifdef WIN32
	LARGE_INTEGER frequency;	/* ticks per second */
	LARGE_INTEGER start;
	LARGE_INTEGER end;
#else
	timeval start;
	timeval end;
#endif
GLfloat gridControlPoints[2][2][3] = {
		{{-1.0, -1.0, 0.0}, {1.0, -1.0, 0.0}},
		{{-1.0, 1.0, 0.0}, {1.0, 1.0, 0.0}}
	};

/********************************************
*            Global functions
*********************************************/
/* Print command line help */
void showHelp() {
	printf( "\n" );
	printf( "Usage: GameOfLife -f PATH [-l RULE] [ADV OPTIONS] WIDTH [HEIGHT]\n");
	printf( "  or:  GameOfLife -r DENSITY [-l RULE] [ADV OPTIONS] WIDTH [HEIGHT]\n");
	printf( "\n" );
	printf( "---- Options ----\n" );
	printf( " -h            Prints this help\n");
	printf( " -f FILE       Path to RLE-file used for starting population\n");
	printf( " -r DENSITY    Use random starting population with given density\n");
	printf( " -l RULE       rule for next generations as a list of Survival/Birth\n");
	printf( "               default: 23/3\n");
	printf( "               defintion is overwritten when there is a\n");
	printf( "               rule specified in the file\n");
	printf( "\n" );
	printf( "---- Advanced OpenCL Options ----\n" );
	printf( " -c            Use clamp mode for images\n");
	printf( "               default: wrap mode\n");
	printf( " -x NUMBER     threads per block for x\n");
	printf( "               default: 32\n");
	printf( " -y NUMBER     threads per block for y\n");
	printf( "               default: 12\n");
	printf( "\n" );
}

/* Read commandline arguments */
int readArguments(int argc, char **argv) {
	
	int optionChar;
	int fSet=0, rSet=0, lSet=0, cSet=0;
	string x(""),y("");
	extern char *optarg;
	extern int optind, optopt;
	
	while ((optionChar = getopt(argc, argv, ":hf:l:r:cx:y:")) != -1) {
		switch (optionChar) {
		case 'f':			/* Set filename */
			if (rSet) {
				fprintf(stderr,"\n-f and -l are mutually-exclusive\n");
				return -1;
			} else {
				GameOfLife.setFilename(optarg);
				fSet++;
			}
			break;
		case 'r':			/* Set density for random mode */
			if (fSet) {
				fprintf(stderr,"\n-f and -l are mutually-exclusive\n");
				return -1;
			} else {
				if (atof(optarg) <= 0.0f) {
					fprintf(stderr,"\nError in density\n");
					return -1;
				}
				GameOfLife.setPopulation(atof(optarg));
				rSet++;
			}
			break;
		case 'l':			/* Set rule */
			if (GameOfLife.setRule(optarg) != 0) {
				fprintf(stderr,"\nError in rule definition\n");
				return -1;
			} else {
				lSet = 2;
			}
			break;
		case 'c':			/* Set clamp mode for images */
			cSet++;
			break;
		case 'x':			/* Set work-items per work group for x */
			x.append(optarg);
			break;
		case 'y':			/* Set work-items per work group for y */
			y.append(optarg);
			break;
		case 'h':
			return -1;
		case ':':			/* -f -l -r -k without operand */
			fprintf(stderr,"\nOption -%c requires an operand\n", optopt);
			return -1;
		case '?':
			fprintf(stderr,"\nUnrecognized option: -%c\n", optopt);
			return -1;
		}
	}
	
	if (fSet == 0 && rSet == 0) {
		fprintf(stderr,"\nNo spawn mode specified\n");
		return -1;
	}
	if (lSet == 0) {
		char defaultRule[] = "23/3";
		GameOfLife.setRule(defaultRule);
	}
	GameOfLife.setKernelBuildOptions(cSet,x,y);
	
	/* Get width and height */
	switch (argc-optind) {
	case 1:
		GameOfLife.setSize(atoi(argv[optind]),atoi(argv[optind]));
		break;
	case 2:
		GameOfLife.setSize(atoi(argv[optind]),atoi(argv[optind+1]));
		break;
	default:
		fprintf(stderr,"\nNo width and/or height specified\n");
		return -1;
	}
	
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
	printf("Game preferences:\n");
	printf("rule: %s | mode: %s | width: %i | height: %i \n",
			GameOfLife.getRule().c_str(),
			GameOfLife.isFileMode() ? "file" : "random",
			GameOfLife.getWidth(), GameOfLife.getHeight());
	printf("Kernel info: \n");
	printf("%s\n",GameOfLife.getKernelInfo().c_str());
	printf("\n");
	printf("Controls:\n");
	printf(" key  | state | description\n");
	printf("------|-------|------------\n");
	printf(" + -  |  %.1f  | wait time between calcuations in sec\n",
					sleeperBarrier / 1000.0f);
	printf("space | %s | start/stop calculation of next generation\n",
					GameOfLife.isPaused() ? " stop" : "start");
	printf("  a   | %s | read image of next generation asynchronously \n",
					GameOfLife.isReadSync() ? " sync" : "async");
	printf("  c   | %s | calculate next generation with CPU/OpenCL \n",
					GameOfLife.isCPUMode() ? " CPU " : " CL  ");
	printf("  g   | %s | draw grid for board\n",
					drawGrid ? " on  " : " off ");
	printf("  r   | %s | resets to starting population on the next stop \n",
					resetGame ? " on  " : " off ");
	printf("  s   | %s | stop after calculation of every generation\n",
					GameOfLife.isSingleGeneration() ? " yes " : " no  ");
	printf("q/esc |       | quit\n\n");
}

/* Free host memory */
void freeMem(void) {
	if (glTex) {
		glDeleteTextures(1, &glTex);
		glTex = 0;
	}
	if (glPBO) {
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, glPBO);
		glDeleteBuffers(1, &glPBO);
		glPBO = 0;
	}
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	if (glShader) {
		glDeleteProgramsARB(1, &glShader);
	}
	if (GLUTWindowHandle)
		glutDestroyWindow(GLUTWindowHandle);
}

/* Get current time in milliseconds */
inline float getCurrentTime() {
#ifdef WIN32
	QueryPerformanceCounter(&end);
	return ( endCount.QuadPart * (1000.0f / frequency.QuadPart)
			 - startCount.QuadPart * (1000.0f / frequency.QuadPart)
			);
#else
	gettimeofday(&end, NULL);
	return ( (float)(end.tv_sec - start.tv_sec) * 1000.0f
			 + (float)(end.tv_usec - start.tv_usec) / 1000.0f
			);
#endif
}

/* Reset timer */
inline void resetTime() {
	#ifdef WIN32
		QueryPerformanceFrequency(&frequency);
		QueryPerformanceCounter(&start);
	#else
		gettimeofday(&start, NULL);
	#endif
}

/********************************************
*         GLUT callback functions
*********************************************/
/* functions which change the board before displaying it */
void displayFunctions() {
	if(!GameOfLife.isPaused() && getCurrentTime() >= sleeperBarrier) {
		resetTime();
	/*
	 * Calculate next generation if game is not paused
	 */
		/*
		 * Map buffer to host memory space
		 * and return address of buffer in host address space
		 *
         * Note that glMapBufferARB() causes sync issue.
		 * If GPU is working with this buffer, glMapBufferARB() will wait(stall)
         * for GPU to finish its job. To avoid waiting (stall), you can call
         * first glBufferDataARB() with NULL pointer before glMapBufferARB().
         * If you do that, the previous data in PBO will be discarded and
         * glMapBufferARB() returns a new allocated pointer immediately
         * even if GPU is still working with the previous data.
		 */
		glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB,
						4*GameOfLife.getWidth()*GameOfLife.getHeight(),
						0, GL_STREAM_DRAW_ARB);
		GLubyte* bufferImage =
			(GLubyte *)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
		
		if (bufferImage) {
		  	/* Write image of next generation directly on the mapped buffer */
			int state = GameOfLife.nextGeneration(bufferImage);
			
			/* Release the mapped buffer */
			glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
			
			if (state != 0) exit(-1);
		}
		
	} else if (GameOfLife.isPaused() && resetGame) {
	/*
	 * Reset game if game is paused
	 */
		glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, 4*GameOfLife.getWidth()*GameOfLife.getHeight(), 0, GL_STREAM_DRAW_ARB);
		GLubyte* bufferImage = (GLubyte *)glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
		
		if (bufferImage) {
		  	/* Write image of next generation directly on the mapped buffer */
			int state = GameOfLife.resetGame(bufferImage);
			
			/* Release the mapped buffer */
			glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
			
			if (state != 0) exit(-1);
		}
		
		
		resetGame = false;
		showControls();
	}
}

/* Display function */
void display() {
	/* Bind the texture and PBO */
	glBindTexture(GL_TEXTURE_2D, glTex);
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, glPBO);
	
	/* Copy pixels from PBO to texture object */
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
					GameOfLife.getWidth(), GameOfLife.getHeight(),
					GL_RGBA, GL_UNSIGNED_BYTE, BUFFER_DATA(0));
	
	/*
	 * Execute functions which change the board
	 */
	displayFunctions();
	
	/*
	 * Release PBOs with ID 0 after use
	 * Once bound with 0, all pixel operations behave normal ways
	 */
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	
	/*
	 * Draw GameOfLife image/board
	 */
	/* Clear buffer */
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	
	/* Save the initial ModelView matrix before modifying it */
    glPushMatrix();
	glLoadIdentity();
	
	/* Transform camera */
	glScalef(cameraDistance,cameraDistance,1);		// zoom
	glTranslatef(verticalMove,horizontalMove,0.0f);	// move
	
	
	/* Draw textured geometry */
	glColor3f(1.0f, 1.0f, 1.0f);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, 1.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, 1.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, -1.0f);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, -1.0f);
	glEnd();
	
	/* Unbind texture */
	glBindTexture(GL_TEXTURE_2D, 0);

	
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
			0, GameOfLife.getWidth(),	/* Starting at 0 mesh width steps. */
			0, GameOfLife.getHeight());	/* Starting at 0 mesh height steps. */
		glDisable(GL_MAP2_VERTEX_3);
	}
	
	glPopMatrix();
	
	glutSwapBuffers();
	glutReportErrors();

	/* Append execution information to window title */
	snprintf(title, 57*sizeof(char)+sizeof(float)+sizeof(int)+sizeof(long),
			 		"Game of Life @ %s @ %f ms/gen @ %i gens/copy @ generation %lu",
					GameOfLife.isCPUMode() ? "CPU" : "OpenCL",
					GameOfLife.getExecutionTime(),
					GameOfLife.getGenerationsPerCopyEvent(),
					GameOfLife.getGenerations()
					);
	glutSetWindowTitle(title);
}

/* Idle function */
void idle(void) { glutPostRedisplay(); }

/* Keyboard function */
void keyboard(unsigned char key, int mouseX, int mouseY) {
	switch(key) {
		/* Pressing space starts/stops calculation of next generation */
		case ' ': GameOfLife.switchPause(); break;
		/* Pressing a switches synchronous reading of images on/off */
		case 'a': GameOfLife.switchreadSync(); break;
		/* Pressing c switches CPU mode on/off */
		case 'c': GameOfLife.switchCPUMode(); break;
		/* Pressing g switches grid for Game of Life board on/off */
		case 'g': drawGrid = !drawGrid; break;
		/* Pressing r resets the board to the starting population */
		case 'r': resetGame = !resetGame; break;
		/* Pressing s switches single generation mode on/off */
		case 's': GameOfLife.switchSingleGeneration(); break;
		/* Pressing escape or q exits */
		/* Pressing p switches waiting time between calculations on/off */
		case '+':
			sleeperBarrier += 100.0f;
			break;
		case '-':
			sleeperBarrier = max(0.0f, sleeperBarrier-100.0f);
			break;
		case 27:
		case 'q':
		case 'Q':
			exit(0);
		default: break;
	}
	showControls();
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
	int windowHeight = (int)((float)glutGet(GLUT_SCREEN_HEIGHT)/2.0f);
	int windowWidth = windowHeight * ((float)GameOfLife.getWidth()/
	                                  (float)GameOfLife.getHeight());
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
	
	/* Hints */
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	
	/* Enable features */
	glEnable(GL_DEPTH_TEST);			// enables depth testing
	
	glClearColor(36./255., 36./255., 85./255., 1.0);	// background color
	glClearStencil(0);					// clear stencil buffer
	glClearDepth(1.0f);					// depth buffer setup: 0 is near, 1 is far
	glDepthFunc(GL_LEQUAL);				// type of depth test
}

/* Initialise OpenGL Pixel Buffer Objects */
void initOpenGLBuffers() {
	
	/* Delete old texture and PBO */
	if (glTex) {
		glDeleteTextures(1, &glTex);
		glTex = 0;
	}
	if (glPBO) {
		glDeleteBuffers(1, &glPBO);
		glPBO = 0;
	}

    /* Create new texture */
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &glTex);
	glBindTexture(GL_TEXTURE_2D, glTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, GameOfLife.getWidth(), GameOfLife.getHeight(),
					0, GL_RGBA, GL_UNSIGNED_BYTE, GameOfLife.getImage());
	glBindTexture(GL_TEXTURE_2D, 0);
	
	/* Generate new pixel buffer object */
	glGenBuffers(1, &glPBO);
	/* Bind the buffer object */
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, glPBO);
	/* Copy pixel data to the buffer object */
	glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB,
				 GameOfLife.getWidth() * GameOfLife.getHeight() * 4,
				 GameOfLife.getImage(), GL_STREAM_DRAW_ARB);
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
	
	/* Read command line arguments */
	if (readArguments(argc, argv) != 0) {
		showHelp();
		return -1;
	}
	
	/* Setup host/device memory, starting population and OpenCL */
	if(GameOfLife.setup()!=0) return -1;

#ifdef PROFILING
	/* Calculate generations for OpenCL profiler without OpenGL output */
	/*
	resetTime();
	do {
		GameOfLife.nextGeneration(GameOfLife.getImage());
	} while (GameOfLife.getGenerations() < 10);
	cout << getCurrentTime() << endl;
	return 0;
	*/
#else
	/* Show controls for Game of Life in console */
	showControls();
	
	/* Setup OpenGL */
	initDisplay(argc, argv);
	
	/* Start timer */
	resetTime();
	
	/* Display GameOfLife image/board and calculate next generations */
	mainLoopGL();
#endif
	
	return 0;
}
