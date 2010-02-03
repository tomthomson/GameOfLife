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

int GameFile::parse() {
	int c;					/* character read from file */
	bool noHeader = false;  /* header specified */
	int curx;				/* x coordinate being written */
	int cury;				/* y coordinate being written */
	
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
	
	/* Parse pattern */
	/*
	for (repl = 1, ifrepl = 0, curx = cury = 0; ; ) {
		while ((c = getc (file)) == ' ' || c == '\t' || c == '\n');
		switch (c) {
		default:
			if (c >= '0' && c <= '9') {
				repl = (ifrepl ? 10*repl : 0) + (c-'0');
				ifrepl = 1;
				continue;
			} else {
				outputChars[5] = c;
				c = 5;
				break;
			}
		case EOF:
			fprintf (stderr, "Missing end-of-file '!' mark\n");
		case '!':
			if (curx != 0)		// ALWAYS flush last line
				fprintf (dstFile, "\n");
			return;
		case 'b':
			c = 0;
			break;
		case 'o':
			c = 1;
			break;
		case 'x':
			c = 2;
			break;
		case 'y':
			c = 3;
			break;
		case 'z':
			c = 4;
			break;
		case '$':
			for (; repl > 0; ++cury, --repl, curx = 0)
				fprintf (dstFile, "\n");
			continue;
		}

		for (c = outputChars[c]; repl > 0; ++curx, --repl)
			putc (c, dstFile);

		repl = 1;
		ifrepl = 0;
	}
	*/
	
	return 0;
}