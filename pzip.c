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

int size = 0;
int len = 64;
void* f_map = NULL;
int fd = 0;

void* get_Filesize(const char* filename) {
    struct stat st;
    stat(filename, &st);
    size = st.st_size;
}

/* Edit Ideas for Parallel
 * 
 * 1) Memory map file and give each thread a pointer to work with
 * 	Could have a "pipe" that holds the pointers to processed parts
	Then the threads have the index of their pointer and put it in 
	correct location in list when finished. (Give more to faster ones
	because they don't have to wait for other threads to finish)
   2) Go file by file or let threads move to next file?
   3) Need to check how many CPUs and only create that many threads

   Shared data Structures
 	Queue: For threads to take pointers out of
	Processed Array/ Linked List: Where threads put processed lines
	
   Thread Responsibilities
	Open and close files (parent) (Want children to run during this I/O)
	Process actual lines (children)
	Write to output (Parent or child writes own?)
 *
 */
void* init_file(const char* filename) {
	//Open file
	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		printf("pzip: cannot open file\n");
		exit(1);
	}

	//Compute size of file	
	get_Filesize(filename);
	
	f_map = malloc((size/len + 1) * sizeof(void*));

	//Create file pointers for current file
	int i = 0;
	while(offset < size) {
		f_map[i] = mmap(NULL, len, PROT_READ, MAP_SHARED, fd, offset);
		offset += len;
		i++;
	}

	//Gets last sequence if it is less than length
	offset -= len;
	if (offset < size) {
		len = size - offset;
		f_map[i] = mmap(NULL, len, PROT_READ, MAP_SHARED, fd, offset);
	}
}


int num_CPUS;
/**
 * This is the main method for the pzip.c file. It houses the main
 * algorithm code and does the file io.
 *
 * @param argc: Holds the number of arguements passed into the program
 * @param argv: Holds the arguments passed into the program
 * @return 0 on successful completion
 */
int main(int argc, char *argv[]) {
	//checks that arguement is passed in
        if (argc < 2) {
                printf("pzip: file1 [file2 ...]\n");
                exit(1);
        }

	//Loops through each incoming file and un-zips them
	for (int i = 1; i < argc; i++) {
			
		//Opens file and maps it with mmap
		init_file(argv[i]);
		
		//Determines number of CPUS (it might change from file to file)
		num_CPUS = get_nprocs();
		pthread_t threads[num_CPUS];

		//Creates threads
		for (int i = 0; i < num_CPUS; i++)
			pthread_create(&threads[i], NULL, process, i);	
	
	
		//Ensures successful file close
		if (close(fd) != 0) {
			printf("Unable to close file.");
			exit(1);
		}
        
	}
	return 0;
}
