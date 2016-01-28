#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	int words = 0; // counts words
	int lines = 0; // counts lines
	int bytes = 0; // counts bytes
	int isNewWord = 1; // determines if the next character is part of a new word

	if (argc == 1) {
	// read from standard input until end of file character is input
		char s[1];
		while (fread(s, 1, 1, stdin)) {
			bytes++;
			if (isNewWord && !isspace(*s)) {
				words++;
				isNewWord = 0;
			} else if (isspace(*s)) {
				isNewWord = 1;
			}
			if (*s == '\n') {
				lines++;
			}
		}
	} else {
	// read from a file

	}

	// print stuff
	printf("      %d       %d       %d\n", lines, words, bytes);
	return 0;
}
