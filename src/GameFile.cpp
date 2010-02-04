#include "../inc/GameFile.hpp"
using namespace std;

int GameFile::open() {
	if ((file = fopen(fileName, "r")) == NULL)
		return -1;
	else
		return 0;
}

int GameFile::skipWhiteSpace() {
	while ((c = getc(file)) == ' ' || c == '\t' || c == '\n' || c == EOF) {
		if (c == EOF)
			return -1;
	}
	return 0;
}

int GameFile::getNumber() {
	int number = 0;
	bool isNumber;		/* last character was a number */
	while (c >= '0' && c <= '9') {
		number = (isNumber ? 10*number : 0) + (c - '0');
		c = getc(file);
	}
	ungetc(c,file);
	return number;
}

bool GameFile::parseHeader() {
	int number = 0;
	if (c != 'x') {
	/* no header line is supplied */
		return true;
	}
	
	/* header line is supplied */
	if (skipWhiteSpace() != 0) return -1;
	
	if (c != '=') return true;
	if (skipWhiteSpace() != 0) return -1;
	
	/* Get width */
	number = getNumber();
	if (number == 0) {
		return true;
	} else {
		width = number;
	}
	if (skipWhiteSpace() != 0) return -1;
	if (c != ',') return -1;
	if (skipWhiteSpace() != 0) return -1;
	if (c != 'y') return -1;
	if (skipWhiteSpace() != 0) return -1;
	if (c != '=') return true;
	if (skipWhiteSpace() != 0) return -1;
	
	/* Get height */
	number = getNumber();
	if (number == 0) {
		return true;
	} else {
		height = number;
	}
	return false;
}

int GameFile::parsePattern() {
	int number = 0;
	bool isNumber = false;
	bool isCell = false;
	int x = 0;
	int y = 0;
	unsigned char color;
	
	for (;;) {
		if (skipWhiteSpace() != 0) return -1;
		switch (c) {
		default:
			if (c >= '0' && c <= '9') {
				number = (isNumber ? 10*number : 0) + (c-'0');
				isNumber = true;
				continue;
			} else {
				return -1;
			}
		case EOF:
			cerr << "Missing end-of-file '!' mark" << endl;
			return -1;
		case '!':
			if (y == height)
				return 0;
			else
				return -1;
		case 'b':
			isCell = true;
			color = 0;
			break;
		case 'o':
			isCell = true;
			color = 255;
			break;
		case 'x':
			isCell = true;
			color = 255;
			break;
		case 'y':
			isCell = true;
			color = 255;
			break;
		case 'z':
			isCell = true;
			color = 255;
			break;
		case '$':
			if (isNumber)
				y += number;
			else
				y++;
			number = 0;
			isNumber = false;
			continue;
		}
		if (isNumber && isCell) {
			for (int i = 0; i < number; x++) {
				pattern[4*x + (4*width*y)] = color;
				pattern[(4*x+1) + (4*width*y)] = color;
				pattern[(4*x+2) + (4*width*y)] = color;
				pattern[(4*x+3) + (4*width*y)] = 1;
			}
			number = 0;
			isNumber = false;
			isCell = false;
		}
	}
	return 0;
}

int GameFile::parse() {
	int c;					/* character read from file */
	bool noHeader = false;  /* header specified */
	
	/* Skip leading comment lines */
	for (;;) {
		c = getc(file);
		switch (c) {
		case EOF:
			return -1;
		case '\n':			/* blank line: ignored */
			continue;
		case '#':			/* #: ignore, but look for EOF in comment */
			while ((c = getc (file)) != '\n') {
				if (c == EOF)	/* EOF in comment */
					return -1;
			}
			continue;
		default:			/* other: legitimate text */
			ungetc(c, file);
			break;
		}
		break;
	}
	
	/* Skip whitespaces */
	if (skipWhiteSpace() != 0) return -1;
	
	/* Check for header line and parse it */
	noHeader = parseHeader();

	if (noHeader)
		return -1;

	pattern = (unsigned char *)malloc(4*width*height*sizeof(char));
	
	/* Parse pattern */
	return parsePattern();
	
	return 0;
}