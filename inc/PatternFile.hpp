#ifndef PATTERNFILE_HPP_
#define PATTERNFILE_HPP_

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <iostream>

class PatternFile {
private:
	FILE                       *file;  /**< FILE for population */
	char                   *fileName;  /**< filename for file */
	unsigned char           *pattern;  /**< parsed pattern */
	int               patternSize[2];  /**< width and height of specified pattern */
	size_t          patternSizeBytes;  /**< size of pattern in bytes */
	std::vector<int>      birthRules;  /**< list of number of neighbours for cell birth */
	std::vector<int>   survivalRules;  /**< list of number of neighbours for cell survival */
	int                            c;  /**< current character being read */

public:
	/** 
	* Constructor.
	* Initialize member variables
	*/
    PatternFile():fileName(NULL) {}
	
    /** 
	* Deconstructor.
	*/
    ~PatternFile() {
		free(fileName);
	}
	
	/** 
	* Parse file.
	*/
	int parse();
	
	/** 
	* Set filename.
	* @param _fileName path to fileName
	*/
    void setFilename(char *_fileName) {
		fileName = (char*)malloc(sizeof(char)*strlen(_fileName)+1);
		memcpy(fileName,_fileName,sizeof(char)*strlen(_fileName)+1);
	}
	
	/**
	* Get parsed pattern.
	* @return pattern
	*/
	unsigned char * getPattern() {
		return pattern;
	}
	
	/**
	* Get rules for birth of a dead cell.
	* @return birthRules
	*/
	std::vector<int> getBirthRules() {
		return birthRules;
	}
	
	/**
	* Get rules for birth of a dead cell.
	* @return survivalRules
	*/
	std::vector<int> getSurvivalRules() {
		return survivalRules;
	}
	
	/**
	* Get width of pattern.
	* @return patternSize[0]
	*/
	int getWidth() {
		return patternSize[0];
	}
	
	/**
	* Get height of pattern.
	* @return patternSize[1]
	*/
	int getHeight() {
		return patternSize[1];
	}
	
private:
	/**
	* Skips comments
	* @return -1 if EOF found, else 0
	*/
	int skipComments();
	
	/** 
	* Skips whitespaces of file
	* @return -1 if EOF found, else 0
	*/
	int skipWhiteSpace();
	
	/** 
	* Get a sequence of numbers and combine them to a number
	* @return number
	*/
	int getNumber();
	
	/** 
	* Parse the header if specified
	* @return true if there is a header, else false
	*/
	bool parseHeader();
	
	/**
	* Parse the pattern
	* @return 0 on success and -1 on failure
	*/
	int parsePattern();
	
	/**
	* Set the state of a cell.
	* @param x x coordinate of cell
	* @param y y coordinate of cell
	* @param state new state of cell
	*/
	void setState(const int x, const int y, const unsigned char state) {
		pattern[4*x + (4*patternSize[0]*y)] = state;
		pattern[(4*x+1) + (4*patternSize[0]*y)] = state;
		pattern[(4*x+2) + (4*patternSize[0]*y)] = state;
		pattern[(4*x+3) + (4*patternSize[0]*y)] = 1;
	}
};

#endif
