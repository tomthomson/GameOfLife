#ifndef __GAMEOFLIFEMAIN_H_
#define __GAMEOFLIFEMAIN_H_

/** 
* Rules for calculating next generation
* 0:	Standard rules by John Conway
  ---------------------------------
	Live cell:
		Survives, if it has two or three live neighbours.
		Dies, if it has one, two or more than thrre live neighbours.
	Dead cell:
		Becomes live cell, if it has exactly three live neighbours.
		Stays dead, if it has not three live neighbours.
	
* 1:	Custom rule
  -----------------
	Live cell:
	Dead cell:
*/
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

#endif