# CS 111 Lab 1 Makefile

CC = gcc
CFLAGS = -g -Wall -Wextra -Wno-unused -Werror
LAB = 1
DISTDIR = lab1-$(USER)

all: 
	$(CC) -std=c99 $(TIMETRASH_SOURCES)
Debug: 
	$(CC) -c $(TIMETRASH_SOURCES)



TIMETRASH_SOURCES = \
  alloc.c \
  execute-command.c \
  main2.c \
  read-command.c \
  print-command.c \
	vector.c \
	stack.c \
	string.c 

clean:
	rm -fr *.o *~ *.bak *.tar.gz core *.core *.tmp

