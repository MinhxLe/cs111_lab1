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
/*
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
                vector_append(dependencies, &v);
            }

            if (c_tree->output != NULL){
                f_dep_t v = checked_malloc(sizeof(struct f_dep));
                f_dep_new(v, c_tree->output, FILE_WRITE, -1);
                vector_append(dependencies, &v);
            }
            
            //1 because its an argument
            for (int i = 1; c_tree->u.word[i] != NULL; i++){
                if (c_tree->u.word[i][0] != '-'){
                    f_dep_t v = checked_malloc(sizeof(struct f_dep));
                    f_dep_new(v, c_tree->u.word[i], FILE_WRITE, -1);
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


//generate a vector of dependencies(level is unnecessary)
//changes depend vector to reflect command_t dependency vector
//return max level
int find_command_level(command_t command, vector_t master_vector){
    //finding dependencies
    int return_level = 0;
    vector_t depend = checked_malloc(sizeof(struct vector));
    vector_new(depend, sizeof(f_dep_t));

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

*/

//void
//levels_vector_new (command_stream_t commands, vector_t levels)
//{
  // levels is a vector containing vectors of commands
  /*
   *generate a vector(levels) of vectors of command_t to execute concurrently in a level)
   */
  // will use a temporary vectors of dependencies
/*
  vector_new(levels, sizeof(vector_t));

  int level = -1;
  command_t command = NULL; 

  vector_t temp_command = NULL;
  
  //master vector
  vector_t command_dep = checked_malloc (sizeof (struct vector));;
  vector_new (command_dep, sizeof (f_dep_t));

  while ((command = read_command_stream (commands)))
    {
      level = find_command_level (command, command_dep);
      if (!vector_get (levels, level, &temp_command))
        {
          vector_t file_dependencies = checked_malloc (sizeof (struct vector));;
          vector_new (file_dependencies, sizeof (command_t));
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
            unsigned int child_count = curr_levels->n_elements;
            pid_t pids[child_count];
            command_t curr = NULL;
            for (unsigned int j = 0; j < child_count; j++){
                vector_get(curr_levels, j, &curr);
                if ((pids[i] = fork()) == -1)
                    error(1,0, "fork error");
                else if (pids[i] == 0){
                    exit(rec_execute_command(curr));
                }
            }
            //parent process waits for all children
            int status2;
            while (child_count > 0){
                wait(&status2);
                if (WEXITSTATUS(status) == WEXITSTATUS(-1))
                    exit(status);
                child_count--; 
            }
            exit(0);
        }
        else{
            waitpid(pid, &status, 0);
            if (WEXITSTATUS(status) == WEXITSTATUS(-1))
                return WEXITSTATUS(status);
        
        } 
    }

    vector_delete(curr_levels);
    free(curr_levels);
    //TODO ERROR BITCH
    //levels_vector_delete(levels);
    free (levels);
    return 0; 
}
)*/
/*
//////////////////////////////////////////////
//              REDOING                 //////
//////////////////////////////////////////////

#define INVALID_DEP -1
#define READ_DEP 0
#define WRITE_DEP 1
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
                f_dep_new(v, c_tree->input, READ_DEP, -1);
                vector_append(dependencies, &v);
            }

            if (c_tree->output != NULL){
                f_dep_t v = checked_malloc(sizeof(struct f_dep));
                f_dep_new(v, c_tree->output, WRITE_DEP, -1);
                vector_append(dependencies, &v);
            }
            
            //1 because its an argument
            for (int i = 1; c_tree->u.word[i] != NULL; i++){
                if (c_tree->u.word[i][0] != '-'){
                    f_dep_t v = checked_malloc(sizeof(struct f_dep));
                    f_dep_new(v, c_tree->u.word[i], WRITE_DEP, -1);
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

struct command_dep{
    command_t command;//command that is to be run
    int rank;
    vector_t dependencies;//vector of ints of pos it depends on
}
typedef struct command_dep* command_dep_t;

void command_dep_new(command_dep_t c, command_t com, int r){
    c->command = com;
    c->rank = r;
    c->dependencies = checked_malloc(sizeof(struct vector));
    vector_new(c->dependencies, sizeof(int));
}

command_dep_delete(command_dep_t c){
    vector_delete(c->dependencies);
    free(c->dependencies);
}


//////
///////////////MASTER DEP NODE IMPLEMENTATION//////////
//////
struct master_dep{
    char* file;
    int curr_dep;
    int curr_dep_type;
    int prev_dep;
}
typedef struct master_dep_t* master_dep_t;

void master_dep_new(master_dep_t master, char* string){
    f->file = checked_malloc(sizeof(char)* (strlen(string) + 1));
    strcpy(master->file,string);
    master->curr_dep = INVALID_DEP;
    master->curr_dep_type = INVALID_DEP;
    master->prev_dep = INVALID_DEP;
}

void master_dep_delete(master_dep_t master){
    free(master->file);
}


void add_dependencies(f_dep_t depend, command_dep_t command_dep, vector_t master_deps){
    master_dep_t curr_mast = NULL;
    int found = 0;
    
    for (unsigned int i = 0; i < master_deps->n_elements;i++){
        vector_get(master_deps, i, &curr_mast);
        if (!strcmp(curr_mast->file, depend->file)){
            //FOUND IN TABLE
            found = 1;
        }
    }
    if (!found){
        //adding new dependency file in the vector
        master_dep_t new = checked_malloc(sizeof(struct master_dep));
        master_dep_new(new, depend->file);
        
    }

}



//creates a new vector of command_deps
void command_dep_vector_new(vector_t command_deps, command_stream_t){
    vector_t master_depend = checked_malloc(sizeof(struct vector));
    vector_new(master_depend, sizeof(struct master_dep_t));
        




    command_t curr_command = NULL;
    vector curr_depend_vect = checked_malloc(sizeof(struct vector));
    vector_new(curr_depend, sizeof(struct f_dep));
    
    f_dep_t curr_depend = NULL;
    while ((curr_command = read_command_stream (commands))){
        //creating a new command_dep
        command_dep_t curr_command_depend = checked_malloc(sizeof(struct command_dep));
        curr_command_depend->command = curr_command;
        
        find_command_dependencies(curr_depend_vect, curr_command);
        //for all dependencies
        for (unsigned int i = 0; i < curr_depend->n_elements; i++){
            vector_get(depend, i, &curr_depend);
            //adding dependencies to command and master depend if necssary
            add_dependencies(curr_depend, curr_command_depend,master_depend);
        }
        
        vector_clear(curr_depend);
    }

    
    vector_delete(curr_depend);
    free(curr_depend);

    vector_delete(master_depend);
    free (master_depend);


}
*/
//////REDOING AGAIN//////
//

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

inline void command_dep_add_dependency(command_dep_t command, vector_t depend){
     
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
    free(fp->prev_command_dep);
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
                    master_file_dep_change_curr(master_curr, command->index, dependency->depend_type);
                }
                //only thing left is concurrent read
                else { 
                    command_dep_add_dependency(command, master_curr->prev_command_dep);
                    vector_append(master_curr-> curr_command_dep, &(command->index));
                }
                found = 1;
    }
    //adding it to master then
    if (!found){
        master_file_dep_change_curr(master_curr, command->index, dependency->depend_type); 
    }
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

                  waitpid (pid[dependence], &status, WNOWAIT);
                  if (WEXITSTATUS (status) == WEXITSTATUS (-1))
                      exit (WEXITSTATUS (status));
              }

              // if it gets here, then all its dependencies must have been completed
              exit (rec_execute_command (command_d->command));
          }
      } // for loop
      exit(0);
  }
  waitpid(child,&status, 0);

    if (WEXITSTATUS (status) == WEXITSTATUS (-1))
        exit (WEXITSTATUS (status));


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
