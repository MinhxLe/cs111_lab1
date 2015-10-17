// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"
#include <unistd.h>
#include <error.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

int
command_status (command_t c)
{
  return c->status;
}

int rec_execute_command(command_t c){
    switch (c->type){
        case(SIMPLE_COMMAND):
            return execvp(c->u.word[0], c->u.word+1);
        case(SEQUENCE_COMMAND):
            rec_execute_command(c->u.command[0]);
            return rec_execute_command(c->u.command[1]);
        case(OR_COMMAND):
            if (rec_execute_command(c->u.command[0]) == -1)
                return rec_execute_command(c->u.command[1]);
            else
                return 0;
        case(AND_COMMAND):
            if (rec_execute_command(c->u.command[0]) == 0)
                return rec_execute_command(c->u.command[1]);
            else
                return -1;
        case(SUBSHELL_COMMAND):
            return rec_execute_command(c->u.subshell_command);
        default:
            return -1;
    }
}


void
execute_command (command_t c, int time_travel)
{
  rec_execute_command(c);
}
