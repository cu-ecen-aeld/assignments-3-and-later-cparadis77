#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>


int main( int argc, char *argv[] )  {

    int num_char = 0;
    FILE *fptr;		// Creating file pointer to work with files

    // Open syslog
    openlog(NULL, 0, LOG_USER);

    // Handle params
    if( argc != 3 ) {
        printf("usage: writer writefile writestr\n");
        printf("  writefile: is a full path to a file (including filename) on the filesystem\n");
        printf("  writestr:  is the text string to write in the file\n");

        // log system LOG_ERR
        syslog(LOG_ERR, "Error! Invalid number of arguments: %d", argc);
        exit(1);
    }
    //else {
    //    // Debug only
    //    printf("Debug only: The program name is %s\n", argv[0]);
    //    printf("Debug only: The arg1 is %s\n", argv[1]);
    //    printf("Debug only: The arg2 is %s\n", argv[2]);
    //}

    // Opening file in writing mode
    fptr = fopen(argv[1], "w");

    if (fptr == NULL) {
        //printf("Error! Unable to open file");
        syslog(LOG_ERR, "Error! Unable to open file");
        exit(1);
    }

    // Write into the file
    num_char = fprintf(fptr, "%s", argv[2]);
    if (num_char > 0)
    {
        syslog(LOG_DEBUG, "Writing %s to %s", argv[2], argv[1]);
    }

    // Close the file
    fclose(fptr);

    return 0;
}
