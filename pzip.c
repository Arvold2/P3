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

	char test = 'c'; //The char that is being compiled
	char curr = 'c'; //The char that is checked to see if same as test
	int count = 1; //Number of same char in a row
	char save = 'c'; //Saves the char carried from previous file

	//Loops through each incoming file and un-zips them
	for (int i = 1; i < argc; i++) {
		
		//Opens file and checks for successful open 
        	FILE *stream = fopen(argv[i], "r");
        	
		if (stream == NULL) {
                	printf("pzip: cannot open file\n");
                	exit(1);
       		}

        	//Reads each line and converts to compressed form
		while ((test = fgetc(stream)) != EOF) {
		
			//Checks if same char continues from previous file
			if (save != test) {
				count = 1;
			} else {
				count++;
			}
	
			//Gets next char and checks if same as previous strings
			curr = fgetc(stream);
			while (curr == test) {
				count++;
				curr = fgetc(stream); //Looks at next char
			}	
		
			//Prints and resets if not match or last file
			if (curr != EOF || i == argc - 1) {
				fwrite(&count, sizeof(int), 1, stdout);
				fprintf(stdout, "%c", test);
				count = 0;
				ungetc(curr, stream);
			} else {
				//Saves the previous char for use in next file
				save = test;
			}
       
		}	
        
		//Ensures successful file close
		if (fclose(stream) != 0) {
			printf("Unable to close file.");
			exit(1);

		}
	}
	return 0;
}
