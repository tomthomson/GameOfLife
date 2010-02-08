#include "../inc/PatternFile.hpp"
using namespace std;

int PatternFile::parse() {
	/* Open pattern file */
	if ((file = fopen(fileName, "r")) == NULL)
		return -2;
	
	bool header = true;  /* header specified */
	
	/* Skip leading comment lines */
	for (;;) {
		c = getc(file);
		switch (c) {
		case EOF:
			fclose(file);
			return -1;
		case '\n':			/* blank line: ignored */
			continue;
		case '#':			/* #: ignore, but look for EOF in comment */
			while ((c = getc (file)) != '\n') {
				if (c == EOF)	/* EOF in comment */
					fclose(file);
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
	if (skipWhiteSpace() != 0) { fclose(file); return -1; }
	
	/* Check for header line and parse it, abort when there is no header */
	header = parseHeader();
	if (!header) { fclose(file); return -1; }
	
	/* Allocate space for pattern according to specified width and height of pattern */
	patternSizeBytes = 4*patternSize[0]*patternSize[1]*sizeof(char);
	pattern = (unsigned char *)malloc(patternSizeBytes);
	
	/* Parse pattern */
	if (parsePattern() != 0) { fclose(file); return -1; }
	
	fclose(file);
	return 0;
}

int PatternFile::skipWhiteSpace() {
	while ((c = getc(file)) == ' ' || c == '\t' || c == 13 || c == 10 || c == EOF) {
		if (c == EOF)
			return -1;
	}
	return 0;
}

int PatternFile::getNumber() {
	int number = 0;
	bool isNumber = false;
	while (c >= '0' && c <= '9') {
		number = (isNumber ? 10*number : 0) + (c-'0');
		isNumber = true;
		c = getc(file);
	}
	ungetc(c,file);
	return number;
}

bool PatternFile::parseHeader() {
	int number = 0;
	/* Check for header line */
	if (c != 'x') return false;
	
	/* Header line is supplied */
	if (skipWhiteSpace() != 0) return false;
	
	if (c != '=') return true;
	if (skipWhiteSpace() != 0) return false;
	
	/* Get width */
	number = getNumber();
	if (number == 0) return false;
	else patternSize[0] = number;
	
	if (skipWhiteSpace() != 0) return false;
	if (c != ',') return false;
	if (skipWhiteSpace() != 0) return false;
	if (c != 'y') return false;
	if (skipWhiteSpace() != 0) return false;
	if (c != '=') return false;
	if (skipWhiteSpace() != 0) return false;
	
	/* Get height */
	number = getNumber();
	if (number == 0) return false;
	else patternSize[1] = number;
	
	/* Get rule definition */
	if (skipWhiteSpace() != 0) return false;
	/*
	 * If there is no , then assume there is no rule definition
	 * Use default Conway rule B3/S23
	 */
	if (c != ',') {
		ungetc(c,file);
		return true;
	}
	
	if (skipWhiteSpace() != 0) return false;
	if (c != 'r') return false;
	if ((c=getc(file)) != 'u') return false;
	if ((c=getc(file)) != 'l') return false;
	if ((c=getc(file)) != 'e') return false;
	if (skipWhiteSpace() != 0) return false;
	if (c != '=') return false;
	if (skipWhiteSpace() != 0) return false;
	
	if (c != 'B') return false;
	/* Get rules for birth of a dead cell */
	while ((c = getc(file)) != '/') {
		switch (c) {
		default:
			if (c >= '0' && c <= '8') {
				birthRules.push_back((int)c-(int)'0');
				break;
			} else {		/* other characters are not allowed */
				return -1;
			}
		case ' ':			/* whitespaces are not allowed */
		case '\t':
		case 13:
		case 10:
		case EOF:			/* end of file is not allowed */
			return true;
		}
	}
	
	if (skipWhiteSpace() != 0) return -1;
	if (c != 'S') return true;
	/* Get rules for survival of a live cell */
	while ((c = getc(file)) != ' ' && c != '\t' && c != 13 && c != 10) {
		switch (c) {
		default:
			if (c >= '0' && c <= '8') {
				survivalRules.push_back((int)c-(int)'0');
				break;
			} else {		/* other characters are not allowed */
				return -1;
			}
		case EOF:			/* end of file is not allowed */
			return true;
		}
	}
	
	return true;
}

int PatternFile::parsePattern() {
	int number = 1;
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
		case ' ':
		case '\t':
		case 13:
		case 10:
			continue;
		case EOF:			/* end of file: not allowed */
			return -1;
		case '!':			/* end of RLE file */
			if (y == (patternSize[1]-1)) {
				/* Fill the rest of the line with dead cells */
				for (int i = x; i < patternSize[0]; i++) {
					setState(x,y,0);
				}
				return 0;
			} else
				return -1;
		case 'b':			/* dead cell */
			isCell = true;
			color = 0;
			break;
		case 'o':			/* live cell */
		case 'x':
		case 'y':
		case 'z':
			isCell = true;
			color = 255;
			break;
		case '$':			/* new line */
			/* Fill the rest of the line with dead cells */
			for (int i = x; i < patternSize[0]; i++) {
				setState(x,y,0);
			}
			x = 0;
			/* Make new line(s) */
			y += isNumber?number:1;
			
			number = 1;
			isNumber = false;
			
			continue;
		}
		if (isCell) {
			/* Make number of c cells */
			for (; number > 0; x++, number--) {
				setState(x,y,color);
			}
			isNumber = false;
			isCell = false;
			number = 1;
		}
	}
}