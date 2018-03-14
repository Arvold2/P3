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
int len = 5;
void** f_map = NULL;
int fd = 0;
void** buffer; //Shared queue between producers and consumers 

//Structure to hold chunks of work
typedef struct {
	int index;
	int size;
	void* block;
} work;


void* get_filesize(const char* filename) {
	struct stat st;
	stat(filename, &st);
	size = st.st_size;
	return NULL;
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
/*
void* init_files(int num_files, char* argv) {
	//Creates a pointer for each file specified		
	for (int file = 0; file < num_files; file++)
		char* filename = argv[file];
		
		//Open file
		fd = open(filename, O_RDONLY);
		if (fd == -1) {
			printf("pzip: cannot open file\n");
			exit(1);
		}
		
		//Compute size of file	
		get_filesize(filename);
		
	
		//Maps to address space and saves the pointer
		void* map_p = mmap(NULL, size, PROT_READ, 0, fd, 0);
		if (map_p == MAP_FAILED) {
			printf("Unable to map file.\n");
			exit(1);
		}

		files[file] = map_p;

		//Create file pointers for current file
		int i = 0, offset = 0;
		while(offset < size) {
			f_map[i] = map_p + offset;
			offset += len;
			i++;
		}

		//Gets last sequence if it is less than length
		offset -= len;
		if (offset < size) {
			len = size - offset;
			f_map[i] = map_p + offset;
		}
		
		f_map = malloc((size/len + 1) * sizeof(void*));
		return NULL;
}
*/
void fill_buf(struct work w) {
	//Add to buffer somehow
	return 0;
}

void consume_buff() {
	//Take ptr out of buffer and start processing it
	//Add to output array when finished
}
//Children enter infinite loop waiting for producer to be finished (Sending num_cpu -1 into buffer)
void *process(void *arg) {
	printf("Child\n");
	consume_buff();
	//Break out of while loop when finds a -1 and consumes it (doesn't leave it for another to find
	return NULL;
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

	//Determines number of CPUS
	num_CPUS = get_nprocs();
	pthread_t threads[num_CPUS];

	//Create consumer threads that wait for work 
	for (int i = 0; i < num_CPUS; i++)
		pthread_create(&threads[i], NULL, process, (void *)(intptr_t)i);	
	
	//Loops through each incoming file and un-zips them
	for (int i = 1; i < argc; i++) {
			
		//Opens file and maps it with mmap
		//init_file(argv[i]);
		
		//Open file
		fd = open(filename, O_RDONLY);
		if (fd == -1) {
			printf("pzip: cannot open file\n");
			exit(1);
		}
		
		//Compute size of file	
		get_filesize(filename);
		
		//Maps to address space and saves the pointer
		void* map_p = mmap(NULL, size, PROT_READ, 0, fd, 0);
		if (map_p == MAP_FAILED) {
			printf("Unable to map file.\n");
			exit(1);
		}
		
		//Parses file and adds work to queue
		for (int i = index; i < (size/len + 1); i++) {
			int work_size;
			void* work_p;
		
			//Check that size doesn't overflow file
			if (i*len > size) {
				work_size = size - (i-1)*len;
			} else {
				work_size = i*len;
			}
			
			work_p = map_p + i - index; //Want current file offset, not ending buffer 
			
			//Add to queue
			work w = {i, work_size, work_p};
			fill_buf(&w);
		}

		//for (int j = 0; j < 1000000; j++);	
		
		//for (int n = 0; n < (size/len + 1); n++)
			//printf("%s\n.", (char*)f_map[n]);	
		
		//Ensures successful file close
		if (close(fd) != 0) {
			printf("Unable to close file.");
			exit(1);
		}
        
	}
	return 0;
}
