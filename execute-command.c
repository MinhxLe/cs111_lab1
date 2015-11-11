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

void file_dep_delete(file_dep_t fp){
    free(fp->file);
}

//finds all file_dep of a command_t tree
void find_command_dependencies(vector_t dependencies, command_t c_tree){
    switch (c_tree->type){
        case(SIMPLE_COMMAND):
            //add io files
            if (c_tree->input != NULL){
                file_dep_t v = checked_malloc(sizeof(struct file_dep));
                file_dep_new(v, c_tree->input, READ_DEP);
                vector_append(dependencies, &v);
            }

            if (c_tree->output != NULL){
                file_dep_t v = checked_malloc(sizeof(struct file_dep));
                file_dep_new(v, c_tree->output, WRITE_DEP);
                vector_append(dependencies, &v);
            }
            
            //1 because its an argument
            for (int i = 1; c_tree->u.word[i] != NULL; i++){
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
struct command_dep{
    command_t command;
    int index;
    vector_t dependencies;
};
typedef struct command_dep* command_dep_t;

void command_dep_new(command_dep_t com_dep, command_t c, int ind){
    com_dep->command = c;
    com_dep->index = ind;
    com_dep->dependencies = checked_malloc(sizeof(struct vector));
    vector_new(com_dep->dependencies, sizeof(int));
}

void command_dep_delete(command_dep_t com_dep){
    vector_delete(com_dep->dependencies);
    free(com_dep->dependencies);
}

void command_dep_add_dependency(command_dep_t command, vector_t depend){
     
    vector_append_vector(command->dependencies, depend);
}

/* master_file_dep struct stores all files used by all command_t so far
 * it also contains the current command that uses it and the type as well
 */
struct master_file_dep{
    char* file;
    vector_t prev_command_dep;
    vector_t curr_command_dep;
    int curr_depend_type;
};
typedef struct master_file_dep* master_file_dep_t;

void master_file_dep_new(master_file_dep_t fp, char* f){
    fp->file = checked_malloc(sizeof(char)* (strlen(f) + 1));
    strcpy(fp->file, f);
    
    fp->prev_command_dep = checked_malloc(sizeof(struct vector));
    fp->curr_command_dep = checked_malloc(sizeof(struct vector));
    vector_new(fp->prev_command_dep, sizeof(int));
    vector_new(fp->curr_command_dep, sizeof(int));
    fp->curr_depend_type = INVALID_DEP;
}

void master_file_dep_delete(master_file_dep_t fp){
    vector_delete(fp->prev_command_dep);
    vector_delete(fp->curr_command_dep);
    free(fp->prev_command_dep);
    free(fp->curr_command_dep);
    free(fp->file);
}

void master_file_dep_change_curr(master_file_dep_t m, int new_dep, int new_dep_type){
    vector_t temp = m->prev_command_dep;
    vector_clear(temp);
    m->prev_command_dep = m->curr_command_dep;
    m->curr_command_dep = temp;
    vector_append(m->curr_command_dep, &new_dep);
    m->curr_depend_type = new_dep_type;
}


void handle_dep(file_dep_t dependency, command_dep_t command, vector_t master_deps ){
    int found = 0;
    master_file_dep_t master_curr = NULL;
    for (unsigned int j = 0; j < master_deps->n_elements; j++){
        vector_get(master_deps,j, &master_curr); 
        //found a match
        if (!strcmp(dependency->file, master_curr->file)){
            //after write or WRITE
            if (master_curr->curr_depend_type == WRITE_DEP || dependency->depend_type == WRITE_DEP){
                //adding new dependency                
                command_dep_add_dependency(command, master_curr->curr_command_dep);
                //changing master
                //master_file_dep_change_curr(master_curr, command->index, dependency->depend_type);
                
                vector_t temp = master_curr->prev_command_dep;
                vector_clear(temp);
                master_curr->prev_command_dep = master_curr->curr_command_dep;
                master_curr->curr_command_dep = temp;
                vector_append(master_curr->curr_command_dep, &(command->index));
                master_curr->curr_depend_type = dependency->depend_type;
                
            }
            //only thing left is concurrent read
            else { 
                command_dep_add_dependency(command, master_curr->prev_command_dep);
                vector_append(master_curr-> curr_command_dep, &(command->index));
            }
            found = 1;
        }

    }
    //adding it to master then
    if (!found){
       master_curr = checked_malloc(sizeof(struct master_file_dep));
       master_file_dep_new(master_curr, dependency->file);
       //master_file_dep_change_curr(master_curr, command->index, dependency->depend_type); 
         
        vector_append(master_curr->curr_command_dep, &(command->index));
        master_curr->curr_depend_type = dependency->depend_type;
       
       
       vector_append(master_deps, &master_curr);
    }
}
    
    

void command_dep_vector_new(vector_t command_dep_vect, command_stream_t commands){
    //master fpd is a vector of all files read so far
    vector_t master_deps = checked_malloc(sizeof(struct vector));    
    vector_new(master_deps, sizeof(master_file_dep_t));
    

    //temporary command_dep needed for the loop(only want to create once)
    command_t curr_command = NULL;
    vector_t curr_command_deps = checked_malloc(sizeof(struct vector));  
    vector_new(curr_command_deps, sizeof(file_dep_t));

    int curr_command_count = 0;
    file_dep_t curr_f_dep = NULL;
    command_dep_t curr_command_dep = NULL;

    while ((curr_command = read_command_stream (commands))){
        //creating a new command_dep for this command
        curr_command_dep = checked_malloc(sizeof(struct command_dep));
        command_dep_new(curr_command_dep, curr_command, curr_command_count);


        //find all dependencies of this current command_t
        find_command_dependencies(curr_command_deps, curr_command);
        //for all dependencies
        for (unsigned int i = 0; i < curr_command_deps->n_elements; i++){

            vector_get(curr_command_deps,i, &curr_f_dep);
             //call a function to check in master dependency vector
            handle_dep(curr_f_dep, curr_command_dep, master_deps);          
        }
        //adding it
        vector_append(command_dep_vect, &curr_command_dep);

        curr_command_count ++;
        vector_clear(curr_command_deps);//clearing vector
    }
    //clearing out master
    master_file_dep_t temp = NULL;
    for (unsigned int i = 0; i < master_deps->n_elements; i++){
        vector_get(master_deps, i,&temp);
        master_file_dep_delete(temp);
        free(temp);
    }

    vector_delete(master_deps);
    free(master_deps);
}

void command_dep_vector_delete(vector_t command_dep){

    command_dep_t temp = NULL;
    for (unsigned int i = 0; i < command_dep->n_elements; i++){
        vector_get(command_dep, i,&temp);
        command_dep_delete(temp);
        free(temp);
    }
    vector_delete(command_dep);
}

int parallel_execute_command_stream(command_stream_t c){
    //create a vector of command_dep_t out of command_stream_t
    command_dep_t command_d = NULL; 

    // command dependencies
    vector_t command_dependencies = checked_malloc (sizeof (struct vector));
    vector_new (command_dependencies, sizeof (command_dep_t));
    command_dep_vector_new (command_dependencies, c);


    // temp variable used in the loop; didn't want to create and destroy multiple times
    int dependence = -1;
    int status = 0;
    // contains all pids of child processes
    pid_t pid[command_dependencies->n_elements];

    // loop through all the commands
    pid_t child;
    if ((child = fork()) == -1)
        error (1, 0, "fork error");
    else if (child == 0){
        
        siginfo_t info; 
        for (unsigned int i = 0; i < command_dependencies->n_elements; i++)
        {
            // parent forks each child
            if ((pid[i] = fork ()) == -1)
                error (1, 0, "fork error");
            else if (pid[i] == 0) // child process
            {
                // get the command_dep at command_dependencies[i]
                vector_get (command_dependencies, i, &command_d);

                // wait on all unfinished dependencies
                // processes can depend on only command trees before them
                // therefore, no check to see if the process has already been forked
                // (all prior processes should already exist)
                for (unsigned int j = 0; j < command_d->dependencies->n_elements; j++)
                {
                    vector_get (command_d->dependencies, j, &dependence);
                    //waitid (P_PID,pid[dependence], &info,WNOWAIT |  WEXITED);
                    //if (WEXITSTATUS (info.si_status) == WEXITSTATUS (-1))
                    //    exit (WEXITSTATUS (info.si_status));
                }

                rec_execute_command (command_d->command);
                exit(0);
            }

        } // for loop
        for (unsigned int j = 0; j < command_dependencies->n_elements; j++)
        {
            waitid (P_PID, pid[j], &info, WNOWAIT | WEXITED);
            if (info.si_code == CLD_EXITED)
              printf("hi");
            printf("%d", info.si_status);
            if (WEXITSTATUS (info.si_status) == WEXITSTATUS (-1))
              exit (WEXITSTATUS (info.si_status));
        }
        exit(0); 


    }
    //parent
    waitpid(child,&status, 0);
    if (WEXITSTATUS (status) == WEXITSTATUS (-1)){
        exit (WEXITSTATUS (status));
    }

    // kill all zombie processes now that they've all completed

    for (unsigned int i = 0; i < command_dependencies->n_elements; i++)
    {
        waitpid (pid[i], &status, 0);
        if (WEXITSTATUS (status) == WEXITSTATUS (-1))
            return -1;
    }
    command_dep_vector_delete (command_dependencies);
    free (command_dependencies);    
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
      parallel_execute_command_stream(c);
  }
}
