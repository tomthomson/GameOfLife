#ifndef GAMEFILE_HPP_
#define GAMEFILE_HPP_

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>

class GameFile {
private:
	FILE        *file;  /**< FILE for population */
	char    *fileName;  /**< filename for file */
	int         width;  /**< width of specified pattern */
	int        height;  /**< height of specified pattern */
	int             c;  /**< current character being read */

public:
	/** 
	* Constructor.
	* Initialize member variables
	*/
    GameFile():
		fileName(NULL)
		{
		
	}
	
    /** 
	* Deconstructor.
	*/
    ~GameFile() {
		free(file);
		free(fileName);
	};
    
	/** 
	* Open filename.
	*/
    int open();
	
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
	* Get width of pattern.
	* @return width
	*/
	int getWidth() {
		return width;
	}
	
	/**
	* Get height of pattern.
	* @return height
	*/
	int getHeight() {
		return height;
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
};

#endif
