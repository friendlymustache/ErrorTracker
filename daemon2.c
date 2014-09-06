#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
const int STDIN = 0;
const int STDOUT = 1;
const int STDERR = 2;


int main(int argc, char* argv[]) {
//  struct termios term;
//  sruct winsize win;
//  forkpty(master, name, &term, &win);
  struct winsize sz;

  ioctl(0, TIOCGWINSZ, &sz);
  printf("Screen width: %i  Screen height: %i\n", sz.ws_col, sz.ws_row);

  int fd;
  char slave_filename[255];
  pid_t pid = forkpty(&fd, slave_filename, &sz, NULL);

  // Handle fork errors
  if (pid == -1) {
    perror("forkpty");
    return 1;
  }

  // Child process - execute bash in child
  else if (pid == 0) {
    if (execlp("/bin/sh", "sh", (void*)0) == -1) {
      perror("execlp");
    }
    fprintf(stderr, "program exited.\n");
    return 1;
  }

  /* Parent process - child process has already exited and
   * won't execute this code
   */
  printf("Child process: %d\n", pid);
  printf("master fd: %d\n", fd);
  printf("Slave filename: %s\n", slave_filename);
  int slave_filedes = open(slave_filename, O_RDWR);  

  if(slave_filedes < 0) {
    perror("open");
    return 1;
  }

  int write_fd = fd;
  int read_fd = fd;

  const char* cmd = "echo $PATH /\n";

  // Writing to <fd> seems to write to the child
  // process which runs the command in bash
  if (write(write_fd, cmd, strlen(cmd)) == -1) {
    perror("write");
    return 1;
  }

  // buffers to store output that's been read
  char buf[255];
  char term_buf[255];

  // number of bytes read from terminal (user input)
  int term_nread;
  int nread;

  // Why is this stuff necessary? 
  // I think because if I don't have it, the terminal itself will read
  // my stdout before I get a chance to
  char *test_buf = " ";
  write(STDIN, test_buf, strlen(test_buf));
  
  while (1) {
    /* Read from terminal stdin (user input)
     * I'm pretty sure this is what blocks and waits for
     * me to enter something
     */
    term_nread = read(STDIN, term_buf, 254);
    // If I've entered something into the terminal, the following
    // stuff is executed
    if(term_nread > 0) {
      // printf("Num of characters read: %d\n", term_nread);
      // Write user input to child bash process
      write(write_fd, term_buf, term_nread);

      // I think there's some delay between when this happens and 
      // when the <read> call below is made, so when I press enter
      // after entering some text, it's written to bash to be processed
      // but the result doesn't come back in time to be read and output
      // to the terminal until the next time around (i.e. we move) to the
      // <read> call basically instantaneously
      sleep(1);

      // Read the output from bash and write it
      // to the terminal window
      nread = read(read_fd, buf, 254);    
      int i;
      for(i = 0; i < nread; i++) {
        putchar(buf[i]);
      }      
      // printf("Finished writing bash output to terminal\n");


    }




  }



  printf("Done\n");
  return 0;
}
