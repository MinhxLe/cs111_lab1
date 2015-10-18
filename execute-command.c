// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <error.h>

#include <stdio.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

int
command_status (command_t c)
{
  return c->status;
}

int rec_execute_command(command_t c){
    switch (c->type){
        case(SIMPLE_COMMAND):{
            pid_t pid;
            int status;
            if ((pid = fork()) < 0){
                //TODO ERROR FORKING
                exit(1);
            }
            else if (pid == 0){//child
                execvp(c->u.word[0], c->u.word);
                exit(-1);
            }
            else{
                //waiting for child to finish
                waitpid(pid, &status, 0);
                return WEXITSTATUS(status); 
            }
        }
        case(SEQUENCE_COMMAND):
            rec_execute_command(c->u.command[0]);
            return rec_execute_command(c->u.command[1]);
        case(OR_COMMAND):
            if (rec_execute_command(c->u.command[0]) == WEXITSTATUS(-1))
                return rec_execute_command(c->u.command[1]);
            else
                return 0;
        case(AND_COMMAND):
            if (rec_execute_command(c->u.command[0]) == WEXITSTATUS(0))
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
