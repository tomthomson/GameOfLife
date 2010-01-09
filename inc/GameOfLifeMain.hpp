#ifndef __GAMEOFLIFEMAIN_H_
#define __GAMEOFLIFEMAIN_H_

/* width and height of the image */
#define WIDTH 1024
#define HEIGHT 1024

/* chance to create a new individual in the starting population */
#define POPULATION 0.3125

/* Initalise display */
void initDisplay(int argc, char * argv[]);

/* Initialise GLUT */
void initGlut(int argc, char * argv[]);

/* Initalise OpenGL */
void initGL(void);

/* OpenGL main loop */
void mainLoopGL(void);

/* Free host/device memory */
void cleanup(void);

#endif