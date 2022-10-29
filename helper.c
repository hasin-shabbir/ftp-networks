#include <stdio.h>
#include <stdlib.h>

int get_file_size(char* fname) {
	FILE *fptr;
    fptr = fopen(fname,"r");
    if (fptr == NULL) {
        perror("File Length Failed.");
        return -1;
    }
	fseek(fptr,0,SEEK_END);
	int file_size = ftell(fptr);
	fclose(fptr);
	return file_size;
}