#ifndef PATTERNFILE_HPP_
#define PATTERNFILE_HPP_

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>

class PatternFile {
private:
	FILE               *file;  /**< FILE for population */
	char           *fileName;  /**< filename for file */
	unsigned char   *pattern;  /**< parsed pattern */
	size_t  patternSizeBytes;  /**< size of pattern in bytes */
	int       patternSize[2];  /**< width and height of specified pattern */
	int                    c;  /**< current character being read */

public:
	/** 
	* Constructor.
	* Initialize member variables
	*/
    PatternFile():fileName(NULL){}
	
    /** 
	* Deconstructor.
	*/
    ~PatternFile() {
		free(fileName);
	};
	
	/** 
	* Parse file.
	*/
	int parse();
	
	/** 
	* Set filename.
	* @param _fileName path to fileName
	*/
    void setFilename(char *_fileName) {
		fileName = (char *)malloc(strlen(_fileName));
		memcpy(fileName, _fileName, strlen(_fileName));
	}
	
	/**
	* Get parsed pattern.
	* @return pattern
	*/
	unsigned char * getPattern() {
		return pattern;
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
	* Skips whitespaces of file
	* @return if EOF found -1, else 0
	*/
	int skipWhiteSpace();
	
	/** 
	* Get a sequence of numbers and combine them to a number
	* @return number
	*/
	int getNumber();
	
	/** 
	* Parse the header if specified
	* @return noHeader
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
