#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <git2.h>
#include <pty.h>
#include <git2.h>
// #include "/usr/lib/libgit2-0.21.1/include/git2.h"

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

void print(char* content) {
    printf("%s\n", content);
}

void fail(char* content) {
    print(content);
    exit(1);
}

int main(int argc, char **argv) {
    pid_t process_id, sid;
    int pdes[2];

    if (pipe(pdes) == -1) {
        exit(EXIT_FAILURE);
    }

    int duplicate_stdin = dup(STDIN_FILENO);
    int duplicate_stdout = dup(STDOUT_FILENO);


    const struct termios *ttyOrigin;
    const struct winsize *winp;
    int *amaster;
    char *name;

    print("About to fork");
    process_id = forkpty(amaster, name, ttyOrigin, winp);

    if(process_id < 0) {
        fail("Failed to fork");
    }

    /* Child process */
    else if(process_id == 0) {
        print("In child process");
    }

    else {
        print("In parent process");
    }

    return 0;

}
