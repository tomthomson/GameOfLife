#ifndef __GAMEOFLIFEMAIN_H_
#define __GAMEOFLIFEMAIN_H_

/* initialise all display related stuff */
void initDisplay(int argc, char * argv[]);

/* initialise GLUT */
void initGlut(int argc, char * argv[]);

/* initalise OpenGL environment */
void initGL (void);

/* program's main loop */
void mainLoopGL(void);

/* Cleanup */
void cleanup(void);

#endif