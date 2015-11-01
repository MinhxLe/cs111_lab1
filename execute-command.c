// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <error.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include "alloc.h"
#include "io_file.h"
#include "vector.h"


void 
handle_io (command_t c)
{
  int read_fd = -1, write_fd = -1;
  // creating a pipe

  if (c->input != NULL)
    {
      if ((read_fd = open (c->input, O_RDONLY)) == -1)
        error (1, 0, "ERROR 6");
      dup2 (read_fd, 0);
    }

  if (c->output != NULL)
    {
      if ((write_fd = open (c->output, O_WRONLY | O_TRUNC | O_CREAT,
                           S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR)) == -1)
          error (1, 0, "ERROR 7");
        
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
            error (1, 0, "ERROR 1");
          else if (pid == 0)
            {//child
              if (strcmp (c->u.word[0], "false") == 0)
                exit (-1);
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
            error (1, 0, "ERROR 2");
          else if (pid == 0)
            {//child
              handle_io (c);
              rec_execute_command (c->u.subshell_command);
              exit (0);
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
            error (1, 0, "ERROR 3");

          pid_t write_pid, read_pid;
          //first we run the write pid code
          if ((write_pid = fork ()) == -1)
            error(1,0,"ERROR 4");
          else if (write_pid == 0)
            {//child write pid
              close (pipefd[0]); //close read pipe 
              dup2 (pipefd[1], 1);
              close (pipefd[1]);//since we already set stdout to be this write fd 
              rec_execute_command (c->u.command[0]);
              exit (0);
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
                error(1,0,"ERROR 5");
              else if (read_pid == 0)
                {
                  close (pipefd[1]);//close write fd
                  dup2 (pipefd[0], 0);//set output pipe as stdin
                  close (pipefd[0]);
                  rec_execute_command (c->u.command[1]);
                  exit (0);
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

////////TODO: ORGANIZE LATER/////////

#define FILE_READ 0
#define FILE_WRITE 1

struct f_dep{
    char * file;//we will need to copy the string from the command_t to 
    
    //we keep track of the current depend type/level based on the most recent
    //use of this file
    int curr_depend_type;
    int curr_level;
};

typedef struct f_dep* f_dep_t;


void f_dep_new(f_dep_t f, char* string, int type, int lvl){
    f->file = checked_malloc(sizeof(char)* (strlen(string) + 1));
    strcpy(f->file, string);
    f->curr_depend_type = type;
    f->curr_level = lvl;
}

void f_dep_new(f_dep_t f){
    free(f->file);

}

void find_command_dependencies(command_t c_tree, vector_t dependencies){
    switch (c_tree->type){
        case(SIMPLE_COMMAND):
            //add io files
            if (c_tree->input != null){
                f_dep_t v = checked_malloc(sizeof(struct f_dep));
                vector_append(dependencies, f_dep_new(v, c_tree->inputs, FILE_READ, -1));
            }

            if (c_tree->output != null){
                f_dep_t v = checked_malloc(sizeof(struct f_dep));
                vector_append(dependencies, f_dep_new(v, c_tree->output, FILE_WRITE, -1));
            }
            
            //1 because its an argument
            for (int i = 1; c_tree->u[i] != NULL; i++){
                if (c_trees->u[i][0] != '-'){
                    f_dep_t v = checked_malloc(sizeof(struct f_dep));
                    vector_append(dependencies, f_dep_new(v, c_tree->input, FILE_WRITE, -1));
                }
            }
            break;
        case (SEQUENCE_COMMAND):
        case (OR_COMMAND):
        case (PIPE_COMMAND):
            find_command_dependencies(c_trees->u.command[0], v);
            find_command_dependencies(c_trees->u.command[1], v);
            break;
        case (SUBSHELL_COMMAND):
            find_command_dependencies(c_trees->u.subshell_command, v);
            break;
    }
}

int find_command_level(command_t command, vector_t depend_vector){
    
    
    
    
//generate a vector of dependencies(level is unnecessary)
//changes depend vector to reflect command_t dependency vector
//return max level
}


void generate_levels_vector(command_stream_t commands, vector_t levels){
    /*
     *generate a vector(levels) of vectors of command_t to execute concurrently in a level)
     */
    //will use a temporary vectors of dependencies
}





void curr_execute_command_stream(){
    //exectues and forks based on level
}




void
execute_command_stream (command_stream_t c, int time_travel)
{
    if (time_travel == 0){
    //original loop
    command_t last_command = NULL;
    command_t command = checked_malloc (sizeof (struct command));

    while ((command = read_command_stream (c)))
    {
          last_command = command;
      	  rec_execute_command (command);
    }
    free(command);
    } 
  else{
  //shit happens
  /*
   *for command trees 
   *
   *
   *
   */
    exit(0); 
  }
}


