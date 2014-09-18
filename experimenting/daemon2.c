#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>

const int STDIN = 0;
const int STDOUT = 1;
const int STDERR = 2;
const int BUF_SIZE = 256;
const int MAX_SLAVENAME = 1000;

int main(int argc, char* argv[]) {
//  struct termios term;
//  sruct winsize win;
  struct termios tty_orig;
  struct winsize ws;
  int master_fd, script_fd;
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


  pid = forkpty(&master_fd, slave_filename, &tty_orig, &ws);

  // Handle fork errors
  if (pid == -1) {
    perror("forkpty");
    return 1;
  }

  // Child process - execute bash in child
  else if (pid == 0) {
    char *shell;
    shell = getenv("SHELL");
    if (shell == NULL || *shell == '\0')
        shell = "/bin/sh";

    execlp(shell, shell, (char *) NULL);
    fprintf(stderr, "program exited.\n");
    return 0;
  }

  /* Parent process - child process has already exited and
   * won't execute this code
   */

   /* Open auxilliary FDs to which we write our output */
    script_fd = open((argc > 1) ? argv[1] : "typescript",
                        O_WRONLY | O_CREAT | O_TRUNC,
                        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |
                                S_IROTH | S_IWOTH);
    if (script_fd == -1)
        perror("open typescript");


    int analyzer_master;

    analyzer_pid = forkpty(&analyzer_master, slave_filename, &tty_orig, &ws);
    if(analyzer_pid == -1) {
      perror("forkpty analyzer");
    }
    else if(analyzer_pid == 0) {
      char *shell;
      shell = getenv("SHELL");
      if (shell == NULL || *shell == '\0')
          shell = "/bin/sh";

      execlp("echo", "echo", "Sup from the second fork bitchezz", (char *) NULL);
      return 0;
    }
    else {
      /* Monitor both pty master and STDIN fds */

      /* Writing to the master side sends data to the slave PTY as input,
       * so the written command is executed by the bash process run in
       * the slave PTY.
       * Writing to STDOUT in the slave side sends the same text to the master fd,
       * where is written as output (so we can read it)
       */

       int read_analyzer = 1;

       while(1) {
        FD_SET(STDIN, &read_set);
        FD_SET(master_fd, &read_set);
        FD_SET(analyzer_master, &read_set);
        if (select(analyzer_master + 1, &read_set, NULL, NULL, NULL) == -1) {
            perror("select");
        }

        if (FD_ISSET(STDIN, &read_set)) {   /* stdin --> pty */
            num_read = read(STDIN, buf, BUF_SIZE);
            if (num_read <= 0)
                exit(EXIT_SUCCESS);

            if (write(master_fd, buf, num_read) != num_read)
                perror("partial/failed write (master_fd)");

            if (write(analyzer_master, buf, num_read) != num_read)
                perror("partial/failed write (master_fd)");

        }

        if (FD_ISSET(master_fd, &read_set)) {      /* pty --> stdout+file */
            num_read = read(master_fd, buf, BUF_SIZE);  
            if (num_read <= 0)
                exit(EXIT_SUCCESS);

            if (write(STDOUT, buf, num_read) != num_read)
                perror("partial/failed write (STDOUT)");
            if (write(script_fd, buf, num_read) != num_read)
                perror("partial/failed write (script_fd)");
        }

        if (read_analyzer && FD_ISSET(analyzer_master, &read_set)) {      /* pty --> stdout+file */
            num_read = read(analyzer_master, buf, BUF_SIZE); 

            char *message = "Reading from analyzer_fd \n";
            write(STDOUT, message, strlen(message));

            if (num_read <= 0)
                read_analyzer = 0;

            if (write(STDOUT, buf, num_read) != num_read)
                perror("partial/failed write (STDOUT)");
            if (write(script_fd, buf, num_read) != num_read)
                perror("partial/failed write (script_fd)");
        }        
       }
     }

char *message = "At end of program \n";
write(STDOUT, message, strlen(message));
  return 0;
}
