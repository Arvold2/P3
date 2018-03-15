///////////////////////////////////////////////////////////////////////////////
// Title:            Program 3 a
// Files:            pzip.c
// Semester:         CS537 Spring 2018
//
// Authors:          Ethan Young, Adam Arvold
// Emails:           Eyoung8@wisc.edu, Arvold2@wisc.edu
//////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/sysinfo.h>
#include <pthread.h>

int size = 0;
int fd = 0;
int len = 5;
int w_index = 0;


//Structure to hold chunks of work
typedef struct {
        int index;
        int size;
        char* block;
} work;

//For buffer
int MAX = 5;
int fillptr = 0;
int numfull = 0;
int useptr = 0;
work** buffer;

//void** buffer; //Shared queue between producers and consumers 


int get_filesize(int filen) {
	struct stat st;
	if (fstat(filen, &st) != 0) {
		printf("Error with stat.");
		exit(1);
	}
	size = st.st_size;
	return 0;
}

void fill_buf(work *w) {
        //Add to buffer somehow
	printf("> %s\n", w->block);
	buffer[fillptr] = w;
	printf("Fill: %d with %s\n", fillptr, w->block);
	fillptr = (fillptr + 1) % MAX;
	numfull++;
}

work* empty_buff() {
	work* tmp = buffer[useptr];
	printf("Empty slot %d that has %s\n", useptr, tmp->block);
	useptr  = (useptr + 1) % MAX;
	numfull--;
	return tmp;
}

int main(int argc, char *argv[]) {
	//checks that arguement is passed in
        if (argc < 2) {
                printf("pzip: file1 [file2 ...]\n");
                exit(1);
        }

	//Inititalizes buffer
	buffer = (work**) malloc(MAX*sizeof(work*));

	//Loops through each incoming file and un-zips them
	for (int i = 1; i < argc; i++) {
		char* filename = argv[i];	
		
		//Open file
		int fd = open(filename, O_RDONLY);
		if (fd == -1) {
			printf("pzip: cannot open file\n");
			exit(1);
		}
		
		//Compute size of file	
		get_filesize(fd);
		
		//Maps to address space and saves the pointer
		char* map_p = mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);
                if (map_p == MAP_FAILED) {
                        printf("Unable to map file.\n");
                        exit(1);
                }
		
                int work_size;
                char* work_p;
		work *w;
		//Parses file and adds work to queue
                for (int i = w_index; i < (size/len + 1); i++) {

                        //Check that size doesn't overflow file
                        if (i*len > size) {
                                work_size = size - (i-1)*len;
                        } else {
                                work_size = i*len;
                        }
			
			//Want current file offset, not ending buffer
                        work_p = map_p + i*len - w_index;  

			w = malloc(sizeof(work));	
                        //Add to queue (TODO: Does this pass whole file or address)
                        w->index = i;
			w->size = work_size;
			w->block = work_p;
                        fill_buf(w);
                }
	
		printf("Buffer[0]: %s\n", (char*)buffer[0]->block);
		printf("Buffer[1]: %s\n", (char*)buffer[1]->block);
		printf("Buffer[2]: %s\n", (char*)buffer[2]->block);
		printf(">> Actual: %s\n",empty_buff()->block);
		printf(">> Actual: %s\n",empty_buff()->block);
		printf(">> Actual: %s\n",empty_buff()->block);
	//	printf("Buffer[3]: %s\n", (char*)buffer[3]->block);

                w_index += size/len;

		//Ensures successful file close
		if (close(fd) != 0) {
			printf("Unable to close file.");
			exit(1);
		}
        
	}
	return 0;
}
