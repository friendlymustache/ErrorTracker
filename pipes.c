#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

const int NBYTES = 1000; 

/* function prototypes */
void die(const char*);
 
int main(int argc, char **argv) {
        /* A quick note on file descriptors: a file descriptor is an index
         * into a table containing the details (permissions, content, size etc)
         * of each open file. File descriptors are typically assigned to the lowest
         * possible unused value each time a new file is opened.
         *
         * Each process has three standard file descriptors:
         * 0 - standard input (stdin)
         * 1 - standard output (stdout)
         * 2 - standard error (stderr)
         *
         * Other descriptors for files opened by the process are assigned as
         * described above. If you close a process, a newly-opened process will
         * reuse the old file descriptor value (beacuse it's the lowest available
         * one).
         */
        
        pid_t child;
        int pdes[2];

        /* Holds duplicate of stdin file descriptor (inherited from terminal) */
        int duplicate_stdin;
        /* Holds duplicate of stdout file descriptor (inherited from terminal) */
        int duplicate_stdout;

        char * buffer = (char *) calloc(NBYTES, sizeof(char));
        int output_code;

        duplicate_stdin = fileno(stdin);
        duplicate_stdout = fileno(stdout);

        pipe(pdes);

        child = fork();
        if(child == (pid_t) (-1)) {
            die("Fork");
        }

        else if(child == (pid_t) 0) {

            /* Close stdin and set stdin to a duplicate of
             * the read-end of the pipe
             */
            close(0);
            int stdin_descriptor;
            
            stdin_descriptor = dup(pdes[0]);

            setpgid(0, 0);
            printf("In child process!");

            /* Read from stdin (whatever was put into the terminal)
             * and write the result (from buffer) to stdout 
             */
            while(read(stdin_descriptor, buffer, NBYTES) != EOF) {
                printf("I'm going ham reading %s", buffer);
            }

            printf("Done with reading stdout...");
            free(buffer);
            exit(0);

        }

        else{

            /* Close stdout and set stdout to a duplicate of
             * the write-end of the pipe
             */
            close(1);
            dup(pdes[1]);

            while(read(duplicate_stdin, buffer, NBYTES) != EOF) {

                /* Write to original stdout */
                write(duplicate_stdout, buffer, NBYTES);

                /* Write to write-end of pipe */
                write(stdout, buffer, NBYTES);
            } 
        }


	return 0;
}
 
void die(const char *msg) {
	perror(msg);
	exit(EXIT_FAILURE);
}