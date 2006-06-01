all: setitimer-helper squish-pty

CC = gcc
CFLAGS = -Wall -W
LDFLAGS = -lm
setitimer-helper: setitimer-helper.o
squish-pty: squish-pty.o

clean: 
	rm -f *.o setitimer-helper squish-pty
