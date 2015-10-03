// UCLA CS 111 Lab 1 command interface
#ifndef COMMAND_H
#define COMMAND_H

#include "command-internals.h"
typedef struct command *command_t;
typedef struct command_stream *command_stream_t;

/* Create a command stream from LABEL, GETBYTE, and ARG.  A reader of
   the command stream will invoke GETBYTE (ARG) to get the next byte.
   GETBYTE will return the next input byte, or a negative number
   (setting errno) on failure.  */
command_stream_t make_command_stream (int (*getbyte) (void *), void *arg);

/* Read a command from STREAM; return it, or NULL on EOF.  If there is
   an error, report the error and exit instead of returning.  */
command_t read_command_stream (command_stream_t stream);

/* Print a command to stdout, for debugging.  */
void print_command (command_t);

/* Execute a command.  Use "time travel" if the integer flag is
   nonzero.  */
void execute_command (command_t, int);

/* Return the exit status of a command, which must have previously been executed.
   Wait for the command, if it is not already finished.   */
int command_status (command_t);
////////PERSONALLY ADDED///////////
void command_new(command_t m_command, enum command_type t, int stat,char* in, char* out, void * args[2]);


void command_stream_new(command_stream_t m_command_stream);
void command_stream_delete(command_stream_t cs);
void command_stream_add(command_stream_t s, command_t* c);

enum io{
 INPUT,
 OUTPUT,
};
void command_set_io(command_t c,char* file,enum io r);


#endif
