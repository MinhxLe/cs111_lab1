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
exec_simple_command(command_t c){
    pid_t pid;
    int status;
    int pipefd[2];
    FILE *read_file, *write_file;
    if (c->input != NULL || c->output != NULL){
        if (pipe(pipefd) == -1){
            //TODO ERROR
            exit(1);
        }
        if (c-> input != NULL)
            if((read_file = fopen(c->input, "r")) == -1){
                //TODO error reading file
                exit(1);
            }
                       
    }

}



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
        
        case (PIPE_COMMAND):{
            int pipefd[2];
            if (pipe(pipefd) == -1){
                //TODO PIPE ERROR
                exit(1);
            }



            pid_t write_pid, read_pid;
            //first we run the write pid code
            if ((write_pid = fork()) == -1){
                //TODO error forking
                exit(1);
            }
            else if (write_pid == 0){//child write pid
                close(pipefd[0]); //close read pipe 
                dup2(pipefd[1], 1);
                close(pipefd[1]);//since we already set stdout to be this write fd 
                rec_execute_command(c->u.command[0]);
            }
            else{//parent process
                int status;
                //waiting for child write process to finish writing
                waitpid(write_pid, &status, 0);
                //error
                if (WEXITSTATUS(status) == WEXITSTATUS(-1)){
                    return WEXITSTATUS(status);
                }

                //handling read process now
                if ((read_pid = fork()) == -1){
                //TODO
                    exit(1);
                }
                else if (read_pid == 0){
                    close(pipefd[1]);//close write fd
                    dup2(pipefd[0], 0);//set output pipe as stdin
                    close(pipefd[1]);
                    rec_execute_command(c->u.command[1]);
                }

                else{//parent again
                    close(pipefd[0]);
                    close(pipefd[1]);
                    waitpid(read_pid, &status, 0);
                    //error
                    if (WEXITSTATUS(status) == WEXITSTATUS(-1)){
                       return WEXITSTATUS(status);
                    } 
                    return 0;
                }
            }
        }
        default:
            return -1;
    }
}


void
execute_command (command_t c, int time_travel)
{
  rec_execute_command(c);
}
