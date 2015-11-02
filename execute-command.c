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
#include "vector.h"
#include "declarations.h"

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

void f_dep_delete(f_dep_t f){
    free(f->file);

}

void find_command_dependencies(vector_t dependencies, command_t c_tree){
    switch (c_tree->type){
        case(SIMPLE_COMMAND):
            //add io files
            if (c_tree->input != NULL){
                f_dep_t v = checked_malloc(sizeof(struct f_dep));
                f_dep_new(v, c_tree->input, FILE_READ, -1);
                vector_append(dependencies,v);
            }

            if (c_tree->output != NULL){
                f_dep_t v = checked_malloc(sizeof(struct f_dep));
                f_dep_new(v, c_tree->output, FILE_WRITE, -1);
                vector_append(dependencies,v);
            }
            
            //1 because its an argument
            for (int i = 1; c_tree->u.word[i] != NULL; i++){
                if (c_tree->u.word[i][0] != '-'){
                    f_dep_t v = checked_malloc(sizeof(struct f_dep));
                    f_dep_new(v, c_tree->input, FILE_WRITE, -1);
                    vector_append(dependencies, v);
                }
            }
            break;
        case (SEQUENCE_COMMAND):
        case (OR_COMMAND):
        case (AND_COMMAND):
        case (PIPE_COMMAND):
            find_command_dependencies(dependencies, c_tree->u.command[0]);
            find_command_dependencies(dependencies, c_tree->u.command[1]);
            break;
        case (SUBSHELL_COMMAND):
            find_command_dependencies(dependencies, c_tree->u.subshell_command);
            break;
    }
}


//generate a vector of dependencies(level is unnecessary)
//changes depend vector to reflect command_t dependency vector
//return max level
int find_command_level(command_t command, vector_t master_vector){
    //finding dependencies
    int return_level = 0;
    vector_t depend = checked_malloc(sizeof(struct vector));
    find_command_dependencies(depend, command);

    //checking master master_vector
     
    f_dep_t command_curr = NULL, master_curr = NULL;
    for (unsigned int i = 0; i < depend->n_elements; i++){
        //TODO use a hashtable...
        vector_get(depend, i, &command_curr);
        for (unsigned int j = 0; j < master_vector->n_elements;j++){
            //getting the 2 elements 
            vector_get(master_vector, j, &master_curr);

            //comparing 2 strings
            if (!strcmp(command_curr->file, master_curr->file)){
                //reading after write
                if (master_curr->curr_depend_type == FILE_WRITE ||command_curr->curr_depend_type == FILE_WRITE){
                    //+1 because you're one level AFTER
                    if (master_curr->curr_level + 1 > return_level)
                        return_level = master_curr->curr_level + 1;
                }
                else{
                    if (master_curr->curr_level > return_level)
                        return_level = master_curr->curr_level;
                }
            }
        }
    }
    //changing master vector
    for (unsigned int i = 0; i < depend->n_elements; i++){
        //TODO use a hashtable...
        vector_get(depend, i, &command_curr);
        
        bool_t found = FALSE;
        for (unsigned int j = 0; j < master_vector->n_elements;j++){
            //getting the 2 elements 
            vector_get(master_vector, j, &master_curr);
                if (!strcmp(command_curr->file, master_curr->file)){
                    master_curr->curr_level = return_level;
                    master_curr->curr_depend_type = command_curr->curr_depend_type; 
                    found = TRUE;
                }
        }
        if (!found){ 
            master_curr = checked_malloc(sizeof(struct f_dep));
            f_dep_new(master_curr, command_curr->file, command_curr->curr_depend_type, return_level );
            vector_set(master_vector, master_vector->n_elements, &master_curr);
        }
    }
    
    
    //freeing all memory   
    for (unsigned int i = 0; i < depend->n_elements; i++){
        f_dep_t curr = NULL;
        vector_get(depend, i, &curr);
        f_dep_delete(curr);
        free(curr);
    }
    vector_delete(depend);
    free(depend);

    return return_level;
}


void
levels_vector_new (command_stream_t commands, vector_t levels)
{
  // levels is a vector containing vectors of commands
  /*
   *generate a vector(levels) of vectors of command_t to execute concurrently in a level)
   */
  // will use a temporary vectors of dependencies

  int level = -1;
  command_t command = checked_malloc (sizeof (struct command));

  vector_t temp_command;
  
  //master vector
  vector_t command_dep = checked_malloc (sizeof (struct vector));;
  vector_new (command_dep, sizeof (struct f_dep));

  while ((command = read_command_stream (commands)))
    {
      level = find_command_level (command, command_dep);
      vector_get (levels, level, &temp_command);
      if (temp_command == NULL)
        {
          vector_t file_dependencies = checked_malloc (sizeof (struct vector));;
          vector_new (file_dependencies, sizeof (struct command));
          vector_set (file_dependencies, file_dependencies->n_elements, &command);

          vector_set (levels, level, &file_dependencies);
        }
      else 
        {
          vector_set (temp_command, temp_command->n_elements, &command);
        }
    }

  vector_delete (command_dep);
  free (command_dep);
}

void levels_vector_delete(vector_t levels){
    vector_t temp = NULL;
    for (unsigned int x = 0; x < levels->n_elements; x++){
        vector_get(levels, x, &temp); 
        vector_delete(temp);
        free(temp);
    }
    vector_delete(levels);
}




int p_execute_command_stream(command_stream_t c){
    //generate all the levels
    vector_t levels = checked_malloc(sizeof(struct vector));
    levels_vector_new(c, levels);


    vector_t curr_levels = checked_malloc(sizeof(struct vector));
    vector_new(curr_levels, sizeof(command_t));

    for (unsigned int i = 0; i < levels->n_elements; i++){
        //getting vector of command_t
        vector_get(levels, i, &curr_levels); 
        //a new process to run all the command_t
        pid_t pid;
        int status;
        
        if ((pid = fork()) == -1)
            error(1,0, "fork error");
        else if (pid == 0){
            //child
            pid_t c_pid;
            
            command_t curr = NULL;
            for (unsigned int j = 0; j < curr_levels->n_elements; j++){
                if ((c_pid = fork()) == -1)
                    error(1,0, "fork error");
                else if (c_pid == 0){
                    vector_get(curr_levels, j, &curr);
                    rec_execute_command(curr);
                    //no need to exit the pid 
                }
                //parent will just continue the loop!
            }
        }
        else{
            waitpid(pid, &status, 0);
            if (WEXITSTATUS(status) == WEXITSTATUS(-1))
                return WEXITSTATUS(status);
        
        }

    
    }

    vector_delete(curr_levels);
    free(curr_levels);

    levels_vector_delete(levels);
    free (levels);
    return 0; 
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
    //TODO ERROR
      p_execute_command_stream(c);
  }
}


