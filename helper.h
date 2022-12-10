#include <stdio.h>
#include <stdlib.h>

//helper to get file size

int get_file_size(char* fname) {
	FILE *fptr;
    fptr = fopen(fname,"r");
    if (fptr == NULL) {
        return -1;
    }
	fseek(fptr,0,SEEK_END);
	int file_size = ftell(fptr);
	fclose(fptr);
	return file_size;
}