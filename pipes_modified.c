#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
 
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


        /* pdes is an initially empty of file descriptors.
         * When pipe is called, data can be written to pdes[1]
         * (write end of pipe) and can then be read from pdes[0]
         * (read end of pipe). 
         * 
         * A process has the file open for reading (correspondingly, writing)
         * if it has a file descriptor that refers to the same file as pdes[0]
         * (correspondingly pdes[1])
         */

	int pdes[2];

        /* Holds child process PID */
	pid_t child;
        
        /* Success code: 0, failure code: -1
         * If creation of pipe fails, print an error message to stderr.
         */
	if(pipe(pdes) == -1)
		die("pipe()");
        
        /* Fork returns twice - first to child process with 0 or -1 for success
         * or failure, then to parent process with child process's PID. Thus, if
         * child == -1 or 0, we know that we're in the child process, and if not we
         * know we're in the parent process. So apparently the child process continues
         * execution of the program where we left off.
         */
	child = fork();
	if(child == (pid_t) (-1))
        	die("fork()"); /* fork failed */
 
	if(child == (pid_t) 0) {
        	/* child process */
 
                /* Stdin is 0, Stdout is 1 for each process.
                 * Close stdout for child process, because child
                 * process is going to share parent's stdout. The
                 * child process's stdin is aready the parent's stdout?
                 */
        	close(1);       

                /* Remember, pdes[1] = file descriptor of write end of pipe */
        	if(dup(pdes[1]) == -1)
        		die("dup()");
 
        	/* now stdout and pdes[1] are equivalent (dup returns lowest free descriptor,
                 *  which happens to be the file descriptor of stdout (1), so our process
                 *  has successfully set the file descriptor of stdout to point towards a duplicate
                 *  of the file descriptor for the write end of the pipe (pdes[1]) ).
                 *  This all works because dup actually modifies the process's state and just returns
                 *  the result (the returned file descriptor)
                */
        
                /* The null here is used to indicate that we aren't passing any more args
                 * (to terminate the arg array)
                 */
        	if((execlp("program1", "program1", "arg1", NULL)) == -1)
        		die("execlp()");
 
		_exit(EXIT_SUCCESS);
	} else {
        	/* parent process */
 
        	close(0);       /* close stdin */
 
        	if(dup(pdes[0]) == -1)
        		die("dup()");
 
        	/* now stdin and pdes[0] are equivalent (dup returns lowest free descriptor) */
 
        	if((execlp("program2", "program2", "arg1", NULL)) == -1)
        		die("execlp()");
 
		exit(EXIT_SUCCESS);
	}
 
	return 0;
}
 
void die(const char *msg) {
	perror(msg);
	exit(EXIT_FAILURE);
}