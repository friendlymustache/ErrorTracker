#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include "create_error_commit.h"

const int STDIN = 0;
const int STDOUT = 1;
const int STDERR = 2;
const int BUF_SIZE = 256;
const int MAX_SLAVENAME = 1000;


void monitor_pty(fd_set *read_set, int pty_master_fd, int analyzer_fd, char* buf) {

      /* Add STDIN, the file descriptor for the master side of the PTY, and
       * the file decsriptor for the terminal-output-analzyer process to
       * the <read_set> file-descriptor-set
       */
      int num_read;
      FD_SET(STDIN, read_set);
      FD_SET(pty_master_fd, read_set);
      FD_SET(analyzer_fd, read_set);

      /* Filter out all file-descriptors that aren't ready to be read 
       * so that read_set only includes file descriptors from which we can read.
       */
      if (select(analyzer_fd + 1, read_set, NULL, NULL, NULL) == -1) {
          perror("select");
      }

      /* Temporary - flag for reading analyzer output */
      int read_analyzer = 1;


      /* If STDIN is in the read_set (if it's ready to be read from),
       * write the (hopefully user-inputted) STDIN text to the pty
       * master FD
       */
      if (FD_ISSET(STDIN, read_set)) {   
          /* Read the STDIN into the buffer */
          num_read = read(STDIN, buf, BUF_SIZE);
          if (num_read <= 0)
              exit(EXIT_SUCCESS);
          /* Write from the buffer to the pty master */
          if (write(pty_master_fd, buf, num_read) != num_read)
              perror("partial/failed write (pty_master_fd)");

          /* Write from the buffer to the analyzer fd */
          if (write(analyzer_fd, buf, num_read) != num_read)
              perror("partial/failed write (analyzer_fd)");

      }

      /* If the pty is ready to be read from (if bash produced some output), 
       * read the output and write it to the terminal STDOUT and the analyzer fd
       */

      if (FD_ISSET(pty_master_fd, read_set)) {    
          /* Read the pty (bash) output into buf */
          num_read = read(pty_master_fd, buf, BUF_SIZE);  
          if (num_read <= 0)
              exit(EXIT_SUCCESS);

          /* Write from the buffer to the terminal STDOUT and the analyzer fd */  
          if (write(STDOUT, buf, num_read) != num_read)
              perror("partial/failed write (STDOUT)");
          if (write(analyzer_fd, buf, num_read) != num_read)
              perror("partial/failed write (analyzer_fd)");
      }

      /* Process output from the analyzer fd; currently, we write it to the terminal
       * STDOUT.
       */
      if (read_analyzer && FD_ISSET(analyzer_fd, read_set)) {      
          num_read = read(analyzer_fd, buf, BUF_SIZE); 
          if (write(STDOUT, buf, num_read) != num_read)
              perror("partial/failed write (STDOUT)");
          if (write(analyzer_fd, buf, num_read) != num_read)
              perror("partial/failed write (analyzer_fd)");
      }        

}





void monitor_terminal(fd_set *read_set, int pty_master_fd, int analyzer_fd, char* read_buffer) {
  /* Monitor both pty master and STDIN fds */

  /* Writing to the master side sends data to the slave PTY as input,
   * so the written command is executed by the bash process run in
   * the slave PTY.
   * Writing to STDOUT in the slave side sends the same text to the master fd,
   * where is written as output (so we can read it)
   */

   int read_analyzer = 1;

   /* Run an infinite loop that monitors the file descriptors for the bash shell run
    * in the PTY slave, the terminal-output-analyzer process, and the terminal itself
    */
   while(1) {
    monitor_pty(read_set, pty_master_fd, analyzer_fd, read_buffer);
   }
 
}






int main(int argc, char* argv[]) {
  //  struct termios term;
  //  sruct winsize win;
  struct termios tty_orig;
  struct winsize ws;
  int pty_master_fd, script_fd;
  char slave_filename[MAX_SLAVENAME];
  pid_t pid, analyzer_pid;
  int slave_filedes;
  char buf[BUF_SIZE];
  fd_set read_set;
  int num_read;

  /* Retrieve the attributes of terminal on which we are started */

  if (tcgetattr(STDIN, &tty_orig) == -1)
      perror("tcgetattr");
  if (ioctl(STDIN, TIOCGWINSZ, &ws) < 0)
      perror("ioctl-TIOCGWINSZ");  

  /* Perform a fork and launch a PTY.
   * The child process closes the file-descriptor to the master side
   * of the PTY, and the parent process closes the file-descriptor to the
   * slave side of the PTY
   */
  pid = forkpty(&pty_master_fd, slave_filename, &tty_orig, &ws);

  // Handle fork errors
  if (pid == -1) {
    perror("forkpty");
    return 1;
  }

  // Child process - execute bash in child (slave side of PTY)
  else if (pid == 0) {
    char *shell;
    shell = getenv("SHELL");
    if (shell == NULL || *shell == '\0')
        shell = "/bin/sh";

    execlp(shell, shell, (char *) NULL);
    fprintf(stderr, "program exited.\n");
    return 0;
  }

  /* Parent process only - child process has already exited and
   * won't execute this code
   */

   /* Open auxilliary FDs to which we write our output */
  script_fd = open((argc > 1) ? argv[1] : "typescript",
                      O_WRONLY | O_CREAT | O_TRUNC,
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |
                              S_IROTH | S_IWOTH);
  if (script_fd == -1)
      perror("open typescript");

  /* Initialize variables for forking off the terminal-output-analyzer program in
   * a separate PTY
   */
  int analyzer_fd;

  /* Fork off the PTY process used to run the analyzer */
  analyzer_pid = forkpty(&analyzer_fd, slave_filename, &tty_orig, &ws);

  /* Handle fork errors */
  if(analyzer_pid == -1) {
    perror("forkpty analyzer");
  }

  /* Execute the analyzer process in the PTY slave (child process)
   * and exit
   */
  else if(analyzer_pid == 0) {
    char *shell;
    shell = getenv("SHELL");
    if (shell == NULL || *shell == '\0')
        shell = "/bin/sh";

    execlp("echo", "echo", "Sup from the second fork", (char *) NULL);
    return 0;
  }

  /* Parent process - monitor user input to the terminal and process it.
   * This call (to monitor_terminal) results in an infinite while loop until the terminal itself
   * is closed or this program is terminated
   */
   monitor_terminal(&read_set, pty_master_fd, analyzer_fd, buf);


  return 0;
}


