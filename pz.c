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

int size = 0;
int fd = 0;
int len = 5;
int w_index = 0;
int num_CPUS = 0;

void*** files;

//Locks and condition variables
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t fill = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;

//Structure to hold chunks of work
typedef struct {
        int index;
        int size;
        char* block;
} work;

//Structure to hold processed char runs
typedef struct {
	int count;
	char c;
} run;

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
	size = st.st_size;
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
	for (int i = 1; i < raw->size; i++) {


		while (curr == test) {
			count++;
			i++;	
			if (i >= raw->size)
				break;
			curr++; //Looks at next char
		}
		
		r = malloc(sizeof(run));
	
		//Adds to processed string
		r->count = count;
		r->c = curr;
		proc[proc_size++] = r;
		
		//If processed last char, saves everything and breaks
		if (i >= raw->size) {
			raw->block = (char*)proc; //Need to type cast to run at end
			raw->size = proc_size;
			break;
		}

		curr++;
		test = curr;
		count = 0;
		i++;

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

void *consumer(void *argv) {
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
		zip(w);
	}
		
	return NULL;
}

void *producer(void *argv) {
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
	       
		 //Add to queue 
		w->index = i;
		w->size = work_size;
		w->block = work_p;

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
	buffer = (work**) malloc(MAX*sizeof(work*));
 	files = malloc((argc-1)*sizeof(void*));
	//Determines number of CPUS
        num_CPUS = get_nprocs();	
	
		
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
		files[i] = malloc(size*sizeof(void*));
		
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
		
		//Create consumer threads that wait for work 
		for (int i = 0; i < num_CPUS; i++)
		      pthread_create(&threads[i], NULL, consumer, NULL);

		//Wait for threads to finish
		pthread_join(pid, NULL);
		for (int i = 0; i < num_CPUS; i++)
			pthread_join(threads[i], NULL);

                w_index += size/len;

		//Ensures successful file close
		if (close(fd) != 0) {
			printf("Unable to close file.");
			exit(1);
		}
        
	}
	return 0;
}
