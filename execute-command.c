// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <error.h>
#include <stdio.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>

void 
handle_io (command_t c)
{
  int read_fd = -1, write_fd = -1;
  // creating a pipe

  if (c->input != NULL)
    {
      if((read_fd = open (c->input, O_RDONLY)) == -1)
        {
          error (1, 0, "ERROR SCRUB");
        }
        dup2 (read_fd, 0);
    }

  if (c->output != NULL)
  {
    if((write_fd = open (c->output, O_WRONLY | O_TRUNC | O_CREAT,
                         S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR)) == -1)
      {
        error (1, 0, "ERROR SCRUB");
      }
      
      dup2 (write_fd, 1);// set write_fd to pipe
  }   
  // don't need to close file because execvp will close file for you
}


int
command_status (command_t c)
{
  return c->status;
}

int 
rec_execute_command (command_t c)
{
  switch (c->type)
    {
      case (SIMPLE_COMMAND):
        {
          pid_t pid;
          int status;
          //forking
          if ((pid = fork ()) == -1)
            error (1, 0, "ERROR SCRUB");
          else if (pid == 0)
            {//child
              handle_io (c);
              execvp (c->u.word[0], c->u.word);    
            }
          else 
            {//parent
             //waiting for child write process to finish writing
              waitpid (pid, &status, 0);
              //error
              if (WEXITSTATUS (status) == WEXITSTATUS (-1))
                return WEXITSTATUS (status);
            }
          return 0;
        }
      case (SEQUENCE_COMMAND):
        rec_execute_command (c->u.command[0]);
        return rec_execute_command (c->u.command[1]);
      case (OR_COMMAND):
        if (rec_execute_command (c->u.command[0]) == WEXITSTATUS (-1))
          return rec_execute_command (c->u.command[1]);
        else
          return 0;
      case (AND_COMMAND):
          if (rec_execute_command (c->u.command[0]) == WEXITSTATUS (0))
            return rec_execute_command (c->u.command[1]);
          else
            return -1;
      case (SUBSHELL_COMMAND):
        {
          pid_t pid;
          int status;
          //forking
          if ((pid = fork ()) == -1)
            error (1, 0, "ERROR SCRUB");
          else if (pid == 0)
            {//child
              handle_io (c);
              rec_execute_command (c->u.subshell_command);
            }
          else 
            {//parent
             //waiting for child write process to finish writing
              waitpid (pid, &status, 0);
              //error
              if (WEXITSTATUS (status) == WEXITSTATUS (-1))
                return WEXITSTATUS (status);
            }
          return 0;
        }
      case (PIPE_COMMAND):
        {
          int pipefd[2];
          if (pipe (pipefd) == -1)
            error (1, 0, "ERROR SCRUB");

          pid_t write_pid, read_pid;
          //first we run the write pid code
          if ((write_pid = fork ()) == -1)
            error(1,0,"ERROR SCRUB");
          else if (write_pid == 0)
            {//child write pid
              close (pipefd[0]); //close read pipe 
              dup2 (pipefd[1], 1);
              close (pipefd[1]);//since we already set stdout to be this write fd 
              rec_execute_command (c->u.command[0]);
            }
          else
            {//parent process
              int status;
              //waiting for child write process to finish writing
              waitpid (write_pid, &status, 0);
              //error
              if (WEXITSTATUS (status) == WEXITSTATUS (-1))
                return WEXITSTATUS (status);

              //handling read process now
              if ((read_pid = fork ()) == -1)
                error(1,0,"ERROR SCRUB");
              else if (read_pid == 0)
                {
                  close (pipefd[1]);//close write fd
                  dup2 (pipefd[0], 0);//set output pipe as stdin
                  close (pipefd[0]);
                  rec_execute_command (c->u.command[1]);
                }

              else
                {//parent again
                  close (pipefd[0]);
                  close (pipefd[1]);
                  waitpid (read_pid, &status, 0);
                  
                  //error
                  if (WEXITSTATUS (status) == WEXITSTATUS (-1))
                    return WEXITSTATUS(status);
                  
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
  rec_execute_command (c);
}
