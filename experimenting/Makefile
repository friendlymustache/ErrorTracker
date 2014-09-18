# Compiler
CC = gcc
LFLAGS = -lgit2 -lutil
# Flags for ensuring proper formatting of C code
CFLAGS = -ansi -pedantic -g -Wstrict-prototypes -Wall

all: pipes program1 program2 pipes_modified daemon

daemon2: daemon2.o
	$(CC) daemon2.o -o daemon2 -lutil

daemon2.o: daemon2.c
	$(CC) -c daemon2.c

pipes: pipes.o
	$(CC) pipes.o -o pipes

pipes.o: pipes.c
	$(CC) -c pipes.c

pipes_modified: pipes_modified.o
	$(CC) pipes_modified.o -o pipes_modified

pipes_modified.o: pipes_modified.c
	$(CC) -c pipes_modified.c	

daemon: daemon.o
	$(CC) daemon.o -o daemon $(LFLAGS)

daemon.o: daemon.c
	$(CC) -c daemon.c	

# Program1 dependencies
program1: program1.o
	$(CC) program1.o -o program1

program1.o: program1.c
	$(CC) $(CFLAGS) -c program1.c

# Program2 dependencies
program2: program2.o
	$(CC) program2.o -o program2

program2.o: program2.c
	$(CC) $(CFLAGS) -c program2.c



check: 
	c_style_check pipes.c

clean:
	rm *.o
	rm pipes program1 program2