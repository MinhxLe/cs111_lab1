// UCLA CS 111 Lab 1 command execution
#define _DEFAULT_SOURCE

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
#define INVALID_DEP -1
#define READ_DEP 0
#define WRITE_DEP 1
////////////////////////////////////////////////////
//basic structure to represent a file dependency
////////////////////////////////////////////////////
struct file_dep{
    char* file;
    int depend_type;
};

typedef struct file_dep* file_dep_t;

void file_dep_new(file_dep_t fp, char* f, int type){
    fp->file = checked_malloc(sizeof(char)* (strlen(f) + 1));
    strcpy(fp->file, f);

    fp->depend_type = type;
}

void
file_dep_delete (file_dep_t fp){
    free (fp->file);
}

//finds all file_dep of a command_t tree
void 
find_command_dependencies (vector_t dependencies, command_t c_tree)
{
  switch (c_tree->type)
    {
      case(SIMPLE_COMMAND):
        //add io files
        if (c_tree->input != NULL)
          {
            file_dep_t v = checked_malloc(sizeof(struct file_dep));
            file_dep_new(v, c_tree->input, READ_DEP);
            vector_append(dependencies, &v);
          }

        if (c_tree->output != NULL)
          {
            file_dep_t v = checked_malloc(sizeof(struct file_dep));
            file_dep_new(v, c_tree->output, WRITE_DEP);
            vector_append(dependencies, &v);
          }

        //1 because its an argument
        for (int i = 1; c_tree->u.word[i] != NULL; i++)
          {
            if (c_tree->u.word[i][0] != '-'){
                file_dep_t v = checked_malloc(sizeof(struct file_dep));
                file_dep_new(v, c_tree->u.word[i], READ_DEP);
                vector_append(dependencies, &v);
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






/*
   command dep structure is a structure that represents a command_t to be
   executed, index represents its rank in index(0 is first read, n is last read)
   dependencies is a vector of ranks of command_t that it depends on
   */
struct 
command_dep {
  command_t command;
  vector_t dependors;
  vector_t dependencies;
};

typedef struct command_dep* command_dep_t;

void
command_dep_new (command_dep_t com_dep, command_t c)
{
  com_dep->command = c;
  com_dep->dependencies = checked_malloc(sizeof(struct vector));
  vector_new (com_dep->dependencies, sizeof(command_dep_t));

  com_dep->dependors = checked_malloc(sizeof(struct vector));
  vector_new (com_dep->dependors, sizeof(command_dep_t));
}

//TODO MEMORY LEAK needs to be recursive
void
command_dep_delete (command_dep_t com_dep)
{
  vector_delete (com_dep->dependencies);
  free (com_dep->dependencies);

  vector_delete (com_dep->dependors);
  free (com_dep->dependors);
}

void
command_dep_add_dependency_by_vector (command_dep_t command, vector_t depend)
{
  vector_append_vector (command->dependencies, depend);
}

void 
command_dep_add_dependor (command_dep_t command, command_dep_t depr)
{
  vector_append (command->dependors, &depr);
}


void 
command_dep_remove_dependency (command_dep_t command, command_dep_t remove)
{
  command_dep_t temp = NULL;
  for (unsigned int x = 0; x <  command->dependencies->n_elements;x++)
    {
      vector_get (command->dependencies, x, &temp);
      //TODO LOL
      if (temp == remove)
        vector_remove (command->dependencies,x);
    }
}




/* master_file_dep struct stores all files used by all command_t so far
 * it also contains the current command that uses it and the type as well
 */
struct 
master_file_dep
{
  char* file;
  vector_t prev_command_dep;
  vector_t curr_command_dep;
  int curr_depend_type;
};
typedef struct master_file_dep* master_file_dep_t;

void
master_file_dep_new (master_file_dep_t fp, char* f)
{
  fp->file = checked_malloc (sizeof(char)* (strlen(f) + 1));
  strcpy (fp->file, f);

  fp->prev_command_dep = checked_malloc (sizeof(struct vector));
  fp->curr_command_dep = checked_malloc (sizeof(struct vector));
  vector_new (fp->prev_command_dep, sizeof(command_dep_t));
  vector_new (fp->curr_command_dep, sizeof(command_dep_t));
  fp->curr_depend_type = INVALID_DEP;
}

void
master_file_dep_delete (master_file_dep_t fp)
{
  vector_delete (fp->prev_command_dep);
  vector_delete (fp->curr_command_dep);
  free (fp->prev_command_dep);
  free (fp->curr_command_dep);
  free (fp->file);
}

void 
master_file_dep_change_curr (master_file_dep_t m, int new_dep, int new_dep_type)
{
  vector_t temp = m->prev_command_dep;
  vector_clear (temp);
  m->prev_command_dep = m->curr_command_dep;
  m->curr_command_dep = temp;
  vector_append (m->curr_command_dep, &new_dep);
  m->curr_depend_type = new_dep_type;
}


void 
handle_dep (file_dep_t dependency, command_dep_t command, vector_t master_deps)
{
  int found = 0;
  master_file_dep_t master_curr = NULL;

  command_dep_t idk = NULL;
  for (unsigned int j = 0; j < master_deps->n_elements; j++)
    {
      vector_get (master_deps,j, &master_curr); 
      //found a match
      if (!strcmp (dependency->file, master_curr->file))
        {
          //after write or WRITE
          if (master_curr->curr_depend_type == WRITE_DEP || dependency->depend_type == WRITE_DEP)
            {
              //adding new dependency                
              command_dep_add_dependency_by_vector (command, master_curr->curr_command_dep);

              //adding dependors
              for (unsigned int x = 0; x < master_curr->curr_command_dep->n_elements; x++)
                {
                  vector_get (master_curr->curr_command_dep,x, &idk);
                  command_dep_add_dependor (idk, command);
                } 

              //changing master
              vector_t temp = master_curr->prev_command_dep;
              vector_clear (temp);
              master_curr->prev_command_dep = master_curr->curr_command_dep;
              master_curr->curr_command_dep = temp;
              vector_append (master_curr->curr_command_dep, &(command));
              master_curr->curr_depend_type = dependency->depend_type;
            }
          //only thing left is concurrent read
          else 
            { 
              command_dep_add_dependency_by_vector (command, master_curr->prev_command_dep);

              for (unsigned int x = 0; x < master_curr->prev_command_dep->n_elements; x++){
                  vector_get (master_curr->prev_command_dep,x, &idk);
                  command_dep_add_dependor (idk, command);
                }
              vector_append (master_curr-> curr_command_dep, &(command));
            }
          found = 1;
        }
    } // for
  //adding it to master then
  if (!found)
    {
      master_curr = checked_malloc (sizeof (struct master_file_dep));
      master_file_dep_new (master_curr, dependency->file);

      vector_append (master_curr->curr_command_dep, &(command));
      master_curr->curr_depend_type = dependency->depend_type;

      vector_append (master_deps, &master_curr);
    }
}



void 
command_dep_vector_new (vector_t seed, command_stream_t commands)
{
  //master fpd is a vector of all files read so far
  vector_t master_deps = checked_malloc (sizeof (struct vector));    
  vector_new (master_deps, sizeof(master_file_dep_t));

  //temporary command_dep needed for the loop(only want to create once)
  command_t curr_command = NULL;
  vector_t curr_command_deps = checked_malloc (sizeof (struct vector));
  vector_new (curr_command_deps, sizeof (file_dep_t));

  file_dep_t curr_f_dep = NULL;
  command_dep_t curr_command_dep = NULL;

  while ((curr_command = read_command_stream (commands))){
      //creating a new command_dep for this command
      curr_command_dep = checked_malloc (sizeof (struct command_dep));
      command_dep_new (curr_command_dep, curr_command);


      //find all dependencies of this current command_t
      find_command_dependencies (curr_command_deps, curr_command);
      //for all dependencies
      for (unsigned int i = 0; i < curr_command_deps->n_elements; i++){

          vector_get (curr_command_deps,i, &curr_f_dep);
          //call a function to check in master dependency vector
          handle_dep (curr_f_dep, curr_command_dep, master_deps);          
      }
      //adding only seed
      if (curr_command_dep->dependencies->n_elements == 0)
          vector_append (seed, &curr_command_dep);

      vector_clear(curr_command_deps);//clearing vector
  }
  //clearing out master
  master_file_dep_t temp = NULL;
  for (unsigned int i = 0; i < master_deps->n_elements; i++){
      vector_get (master_deps, i,&temp);
      master_file_dep_delete (temp);
      free (temp);
  }

  vector_delete (master_deps);
  free (master_deps);
}

void 
rec_execute_command_dep (command_dep_t root)
{
  pid_t child;
  int status;
  if ((child = fork()) == -1)
    error (1, 0, "fork error");
  else if (child == 0)
    exit (rec_execute_command (root->command));
  else
    {
      waitpid (child, &status,0);
      if (WEXITSTATUS (status) == WEXITSTATUS (-1))
        error (1, 0, "process failed");
      //removing itself from its dependor list 
      command_dep_t temp = NULL;
      for (unsigned int x = 0; x < root->dependors->n_elements; x++)
        {
          vector_get (root->dependors, x, &temp);
          command_dep_remove_dependency (temp, root);
          if (vector_empty (temp->dependencies))
            rec_execute_command_dep (temp);
        }
      command_dep_delete (root);//might as well delete now    
    }
}


int 
parallel_execute_command_stream (command_stream_t c)
{
  command_dep_t command_d = NULL; 

  // command dependencies
  vector_t seed = checked_malloc (sizeof (struct vector));
  vector_new (seed, sizeof (command_dep_t));
  command_dep_vector_new (seed, c);

  // temp variable used in the loop; didn't want to create and destroy multiple times
  int status = 0;
  pid_t pid[seed->n_elements];
  command_dep_t temp;

  for (unsigned int x = 0; x < seed->n_elements; x++){
      vector_get(seed, x, &temp);
      if ((pid[x] = fork ()) == -1)
          return -1;
      else if  (pid[x] == 0){
          rec_execute_command_dep(temp); 
          exit(0);
      }
  }

  for (unsigned int i = 0; i < seed->n_elements; i++)
  {
      waitpid (pid[i], &status, 0);
      if (WEXITSTATUS (status) == WEXITSTATUS (-1))
        return -1;
  }
  free (seed);    
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
  else
    {
      if (parallel_execute_command_stream (c) == -1)
        error (1, 0, "fork error");
    }
}
