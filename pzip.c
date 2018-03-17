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
//#include "mythread-macros.h"
#define _GNU_SOURCE 
int size = 0;
int fd = 0;
int len = 1000000;
int w_index = 0;
int num_CPUS = 0;

void*** files;
int* file_size;
int file_num = 0;
int num_files = 0;

//Locks and condition variables
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t fill = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;


//Structure to hold processed char runs
typedef struct {
	unsigned long int count;
	char c;
} run;

//Structure to hold chunks of work
typedef struct {
        int index;
        int size;
        char* block;
	run** procs;
} work;

//For buffer
int MAX = 5;
int fillptr = 0;
int numfull = 0;
int useptr = 0;
work** buffer;
char* map_p;

int spin() {
	for (int i = 0; i < 100000000; i++);
	return 0;
}

int get_filesize(int filen) {
	struct stat st;
	if (fstat(filen, &st) != 0) {
		printf("Error with stat.");
		exit(1);
	}
	size = st.st_size - 1;
	return 0;
}

work* zip(work* raw) {
	char test = raw->block[0]; //The char that is being compiled
        char curr = raw->block[0]; //The char that is checked to see if same as test
        int count = 0; //Number of same char in a row
	run** proc = malloc(raw->size*sizeof(run*));
	int proc_size = 0;
	run *r;
	
	//Reads each char and creates the runs
	for (int i = 0; i < raw->size; i++) {
		curr = raw->block[i];
		test = curr;
		
		//Loops until different chars or end of section for processing
		while (curr == test) {
			count++;
			i++;
			
			if (i >= raw->size) {
				break;
			}
			curr = raw->block[i]; //Looks at next char
		}
		
		r = malloc(sizeof(run));
	
		//Adds to processed string
		r->count = count;
		r->c = test;
		proc[proc_size++] = r;
	
		//If processed last char, saves everything and breaks
		if (i >= raw->size) {
			raw->procs = proc; //Need to type cast to run at end
			raw->size = proc_size;
//			printf("proc_size: %d", raw->size);
			break;
		}
		i--;
		count = 0;
	}
		
	return raw;

}
void fill_buf(work *w) {
	buffer[fillptr] = w;
	fillptr = (fillptr + 1) % MAX;
	numfull++;
}

work* empty_buff() {
	work* tmp = buffer[useptr];
	useptr  = (useptr + 1) % MAX;
	numfull--;
	return tmp;
}

int print_results() {
	//int index = 0;
	//char* write_buff = malloc(100000*sizeof(char));
	int tally = 0;
	char c_curr = 'c';
	//printf("File size: %d Num Files: %d\n", file_size[0], num_files);	
	for (int i = 0; i < num_files; i++) { //Each file
//		printf("FIle size: %d\n", file_size[i]);
		for (int j = 0; j < file_size[i]; j++) { //Each "work"
			work* w = (work*)files[i][j];
			//printf("Work size: %d\n", w->size);
			for (int k = 0; k < w->size; k++) { //Each "run"
				run* r = w->procs[k];
				//printf("Incoming: %lu %c for %d\n", r->count, r->c, j);
				//printf("File: %d, work: %d run: %d\n", i, j, k);
		//		printf("Tally: %d c_curr: %c\n", tally, c_curr);
				//printf("%d\n", k);	
	
				//printf("Here");	
				//Initialize
				if (i == 0 && j == 0 && k == 0) {
					tally = r->count;
					c_curr = r->c;
					if (i == num_files - 1 && j == file_size[i] - 1 && k == w->size - 1) {
						printf("%d%c", tally, c_curr);
						//fwrite(&tally, sizeof(int), 1, stdout);
						//fprintf(stdout, "%c", c_curr);
						break;
					}
				
					continue;
				}

				if (c_curr == r->c) {
					tally += r->count;		
					//Prints if last sequence is same as prev
					if (i == num_files - 1 && j == file_size[i] - 1 && k == w->size - 1) {
						printf("%d%c", tally, c_curr);
						//fwrite(&tally, sizeof(int), 1, stdout);
						//fprintf(stdout, "%c", c_curr);
						break;
					}
				
					continue;
				} else {
					printf("?%d%c", tally, c_curr);
					//fwrite(&tally, sizeof(int), 1, stdout);
                                	//fprintf(stdout, "%c", c_curr);
					c_curr = r->c;
					tally = r->count;
				}
				
				//Prints if last sequence is not same as prev
				if (i == num_files - 1 && j == file_size[i] - 1 && k == w->size - 1) {
					printf("|%d%c", tally, c_curr);
					//fwrite(&tally, sizeof(int), 1, stdout);
                                	//fprintf(stdout, "%c", c_curr);
					break;
				}
			}
		}
	}
		
	return 0;
}
void *consumer(void *argv) {
	//int count = 0;
	while(1) {
		pthread_mutex_lock(&m);
		while (numfull == 0)
			pthread_cond_wait(&fill, &m);
		work *w = empty_buff();
		pthread_cond_signal(&empty);
		pthread_mutex_unlock(&m);
		if (w == NULL) {
			break;
		}
	//	count++;
//		printf("Index %d\n", w->index);	
		files[file_num][w->index] = (void*)zip(w);
	}
	//printf("Thread count: %d\n", count);
	return NULL;
}

void *producer(void *argv) {
	int work_size;
	char* work_p;
	work *w;
	//Parses file and adds work to queue
	for (int i = w_index; i < (size/len + 1 + w_index); i++) {
		if (size < len) {
			work_size = size;
			work_p = map_p;
//			printf("Whet the hell");
		}
		//Check that size doesn't overflow file
		else {
			if ((i+1)*len > size) {
				work_size = size - i*len;
			} else {
				work_size = len;
			}
		
			//Want current file offset, not ending buffer
			work_p = map_p + (i)*len - w_index;  
		}

		//Doesn't give null jobs to threads
		if (work_size <= 0) {
			break;
		}
	

		w = malloc(sizeof(work));	
	       
		 //Add to queue 
		w->index = i;
		w->size = work_size;
		w->block = work_p;
//		printf("work size: %d for file: %d\n", w->size, file_num);
		//Aqcuire lock and fill buffer
		pthread_mutex_lock(&m);
		while(numfull == MAX)
			pthread_cond_wait(&empty, &m);	
		fill_buf(w);
		pthread_cond_signal(&fill);
		pthread_mutex_unlock(&m);
	}
	
	//Add ending conditions
	for (int i = 0; i < MAX; i++) {	
		//Aqcuire lock and fill buffer
		pthread_mutex_lock(&m);
		while(numfull == MAX)
			pthread_cond_wait(&empty, &m);	
		fill_buf(NULL);
		pthread_cond_signal(&fill);
		pthread_mutex_unlock(&m);
	}
	return NULL;
}

int main(int argc, char *argv[]) {
	//checks that arguement is passed in
        if (argc < 2) {
                printf("pzip: file1 [file2 ...]\n");
                exit(1);
        }
	
	//Inititalizes buffer
 	files = malloc((argc-1)*sizeof(void**));
	file_size = malloc((argc-1)*sizeof(int));

	if (files == NULL) {
		printf("ERROR MALLOC MAIN TOP.");
		exit(1);
	}
	num_files = argc-1;
	//Determines number of CPUS
	num_CPUS = get_nprocs();	
	//num_CPUS = 4;
	MAX = num_CPUS;		
	//Loops through each incoming file and un-zips them
	for (int i = 1; i < argc; i++) {
		char* filename = argv[i];	
		file_num  = i - 1;
		
		//Open file
		int fd = open(filename, O_RDONLY);
		if (fd == -1) {
			printf("pzip: cannot open file\n");
			exit(1);
		}
		
		//Compute size of file and allocate output array
		get_filesize(fd);
		files[i-1] = malloc(((size/len) + 1)*sizeof(void*));	
		if (files[i-1] == NULL) {
			printf("ERROR MALLOC MAIN MID.");
			exit(1);
		}

		if ((len % 2) == 0) {
			file_size[i-1] = size/len + 1;
		} else {	
			file_size[i-1] = size/len;
		}
	//	printf("%d!", file_size[i-1]);
 
		//Maps to address space and saves the pointer
		map_p = mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);
                if (map_p == MAP_FAILED) {
                        printf("Unable to map file.\n");
                        exit(1);
                }
		//Create producer thread	
		pthread_t pid;
              	pthread_create(&pid, NULL, producer, NULL);
		pthread_t threads[num_CPUS];
		
		buffer = (work**) malloc(MAX*sizeof(work*));
		//Create consumer threads that wait for work 
		for (int j = 0; j < num_CPUS; j++)
		      pthread_create(&threads[j], NULL, consumer, NULL);
			
		//Wait for threads to finish
		pthread_join(pid, NULL);
		for (int k = 0; k < num_CPUS; k++)
			pthread_join(threads[k], NULL);
		free(buffer);
		//Keep index for next file
                w_index = 0;
		
		//Ensures successful file close
		if (close(fd) != 0) {
			printf("Unable to close file.");
			exit(1);
		}
		
	}
		//Combine and print array
       		print_results(); 
	exit(0);
}
