#ifndef __GAMEOFLIFEMAIN_H_
#define __GAMEOFLIFEMAIN_H_

/* width and height of the image */
#define WIDTH 512
#define HEIGHT 512

/* chance to create a new individual in the starting population */
#define POPULATION 0.04

/* Initialise OpenGL Utility Toolkit */
void initGlut(int argc, char * argv[]);

/* Initalise OpenGL Extension Wrangler Library */
void initGlew(void);

/* Initalise display */
void initDisplay(int argc, char * argv[]);

/* OpenGL main loop */
void mainLoopGL(void);

/* Free host/device memory */
void cleanup(void);

#endif