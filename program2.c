#include <stdio.h>
#include <stdlib.h>
const int BUFFSIZE = 100;
/* Added for practice defining stuff */
#define BUFFER_SIZE 100

int main(int argc, char** argv) {
	/* We want to scan in (as a string) the stdout output of program1 */
	/* FILE * file_pointer = fopen("read_me.txt", "r"); */

	char * scan_result = (char *) calloc(BUFFSIZE, sizeof(char));

	
	/* fgets(scan_result, BUFFSIZE, stdin); */
	scanf("%s", scan_result);
	printf("Printing to stdout for program 2: %s with %d command line arguments", scan_result, argc);
	return 0;
}