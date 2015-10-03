// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"


////////////////////////////////////////////
//             INCLUDE FILES         ///////
////////////////////////////////////////////

#include "alloc.h"

#include <error.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#define FALSE 0
#define TRUE 1
typedef unsigned short bool_t;
////////////////////////////////////////////
////////VECTOR CLASS IMPLEMENTATION ////////
////////////////////////////////////////////
typedef struct vector* vector_t;
struct vector{
  size_t ELEMENT_SIZE; // in bytes
  size_t n_elements;
  size_t N_MAX_ELEMENTS;
  void* elements;
};

void vector_new(vector_t s, size_t es){
  s->ELEMENT_SIZE = es;
  s->n_elements = 0;//not really important, just signifies theright most element that has been changed
  s->N_MAX_ELEMENTS = 8;
  s->elements = (void*)checked_malloc(es * s->N_MAX_ELEMENTS);
}

void vector_delete(vector_t s){
  free(s->elements);
}


void vector_set(vector_t s, size_t index, void* source){
  while (index >= s->N_MAX_ELEMENTS){
    s->N_MAX_ELEMENTS *= 2; 
    s->elements = (void*)checked_realloc(s->elements, s->ELEMENT_SIZE * s->N_MAX_ELEMENTS);
  }
  void* offset = (void*)((char*)s->elements + index*s->ELEMENT_SIZE); //converting to byte size for (char*)
  memcpy(offset, source, s->ELEMENT_SIZE);
  s->n_elements = index + 1;
}

int vector_get(vector_t s, size_t index, void* dest){
  if (index < s->n_elements){//not a very good check
    void* offset = (void*)((char*)s->elements + index*s->ELEMENT_SIZE); //converting to byte size for (char*)
    memcpy(dest, offset, s->ELEMENT_SIZE);
    return 1;
  }
  return 0;
}


unsigned int vector_remove(vector_t s, size_t index){
  if (index >= s->n_elements)
    return 0;
  else if (index == s->N_MAX_ELEMENTS - 1)
    s->N_MAX_ELEMENTS -= 1;
  else{
    char* offset = ((char*)s->elements + index*s->ELEMENT_SIZE); //converting to byte size
    memcpy((void*)offset, (void*)(offset + s->ELEMENT_SIZE), (s->n_elements-1 - index)*s->ELEMENT_SIZE);  
    s->n_elements -= 1;    
  }
  return 1;
}


////////////////////////////////////////////
////////   STACK IMPLEMENTATION    /////////
////////////////////////////////////////////
typedef struct stack* stack_t;
struct stack{
  vector_t v_stack;
  size_t n_elements;
};

void stack_new(stack_t s, size_t element_size){
  s->n_elements = 0;
  s->v_stack = (vector_t)checked_malloc(sizeof(struct vector));
  vector_new(s->v_stack, element_size);
}



void stack_delete(stack_t s){
  vector_delete(s->v_stack);
  free(s->v_stack);
}

int stack_pop(stack_t s, void* dest){
  if (s->n_elements == 0)
    return 0;
  
  s->n_elements--;
  vector_get(s->v_stack, s->n_elements, dest); //returns the pointer
  vector_remove(s->v_stack, s->n_elements); 
  return 1;
}

void stack_push(stack_t s, void* item){
  vector_set(s->v_stack, s->n_elements, item);
  s->n_elements++;
}

bool_t stack_top(stack_t s, void* dest){
  if (s->n_elements == 0)
    return FALSE;
  return vector_get(s->v_stack, s->n_elements-1, dest);
}

bool_t stack_empty(stack_t s){
  return (s->n_elements == 1);
}

////////////////////////////////////////////
////////   STRING IMPLEMENTATION    ////////
////////////////////////////////////////////


typedef struct string* string_t;
struct string{
  size_t length;
  vector_t v_string;
};

void string_new(string_t s){
  s->length = 0;
  s->v_string = (vector_t)checked_malloc(sizeof(struct vector));
  vector_new(s->v_string,sizeof(char));
}
void string_new_cstring(string_t s, char* str){
  s->length = 0;
  s->v_string = (vector_t) checked_malloc(sizeof(struct vector));
  vector_new(s->v_string,sizeof(char));

  while (str[s->length] != '\0'){
    vector_set(s->v_string, s->length, str + s->length);
    s->length++;
  }
}

void string_delete(string_t s){
  vector_delete(s->v_string);
  free(s->v_string);
}


void string_append(string_t s, char* str){
  int counter = 0;
  while (str[counter] != '\0'){
    vector_set(s->v_string, s->length, str + counter);
    s->length++;
    counter++;
  }
}
//TODO CHECK THIS
void string_append_char(string_t s, char ch){
  vector_set(s->v_string, s->length, &ch);
  s->length++;
}



int string_find_substring(string_t s, char* str){
  //TODO
  return -1;
}

//this takes in a pointer and 
void string_to_cstring(string_t s, char* dest){
  size_t x;
  for (x = 0; x < s->length; x++)
    vector_get(s->v_string,x, dest+x);
  dest[x] = '\0';//setting null byte
}

void cstring_to_string(string_t dest, char* source){
  dest->length= 0; //clearing string
  while (source[dest->length] != '\0'){
    vector_set(dest->v_string, dest->length, source+dest->length);
    dest->length++;
  }
}
  


//////COMMAND CONSTRUCTOR///////////
void command_new(command_t m_command, enum command_type t, int stat, 
    char* in, char* out, void * args[2]){
  
  //TODO: check what type,status, and ioredirection is
  m_command->type = t;
  m_command->status = stat;
  m_command->input = in;
  m_command->output = out;

  switch (m_command->type){
    case (AND_COMMAND):
    case (OR_COMMAND):
    case (PIPE_COMMAND):
    case (SEQUENCE_COMMAND):
      //you're passing in a pointer to pointer of commands
      //make sure that that pointer is pointing to a 2 unit array
      m_command->u.command[0] = (command_t)args[0];
      m_command->u.command[1] = (command_t)args[1];
      break;
    case (SIMPLE_COMMAND):
      m_command->u.word = (char**)args[0];
      break;
    case (SUBSHELL_COMMAND):
      m_command->u.subshell_command = (command_t)args[0];
  }
}
//TODO DECONSTRUCTOR FOR COMMAND
//MIGHT NOT NEED TO SINCE NO MALLOCING

/////////////////////////////////////////////////
///////COMMAND STREAM IMPLEMENTATION/////////////
/////////////////////////////////////////////////
//command stream implementation
struct command_stream{
  //points to list of ROOTS of command tree
  vector_t command_trees;
  unsigned int n_commands;
};

//initalizes new command_stream
void command_stream_new(command_stream_t m_command_stream){
  //creating initial command_stream
  m_command_stream->n_commands = 0;
  m_command_stream->command_trees = malloc(sizeof (struct vector));
  vector_new(m_command_stream->command_trees, sizeof(command_t*));
}

void command_stream_delete(command_stream_t cs){
 free(cs->command_trees); 
}

//COPIES
void command_stream_add(command_stream_t s, command_t* c){
  vector_set(s->command_trees, s->n_commands, c);
}
///////////////////////////////////////////////
////////HELPER FUNCTIONS //////////////////////
///////////////////////////////////////////////

/*
 *pops 2 commnad from command stack, and creates a new command and
 *combines with popped stack, pushes them onto stack
 */
bool_t opp_pop_from_stack(stack_t command_stack, stack_t opp_stack){
  //both of them shouldn't be empty?
  if (stack_empty(command_stack) || stack_empty(opp_stack))
    return FALSE;
  
  command_t commands[2];
  char* opperand;
  stack_pop(command_stack, commands);
  stack_pop(command_stack, commands+1);
  stack_pop(opp_stack, &opperand);
  command_t dest = checked_malloc(sizeof(struct command)); 
  if (strcmp(opperand, "&&") == 0){
    //TODO: what is status
    command_new(dest, AND_COMMAND, -1, NULL, NULL, (void*)commands);  
  }

  else if (strcmp(opperand, "||") == 0){
    command_new(dest, OR_COMMAND, -1, NULL, NULL, (void*)commands);  
  }
  else if (strcmp(opperand, ";") == 0){
    command_new(dest, SEQUENCE_COMMAND, -1, NULL, NULL, (void*)commands);  
  }
  else if (strcmp(opperand, "|") == 0){ 
    command_new(dest, PIPE_COMMAND, -1, NULL, NULL, (void*)commands);  
  }
  else{
    printf("YOU DUN GOOFED\n");
    free(dest);
    return FALSE;
  }
  stack_push(command_stack, &dest);
  return TRUE;
}

/*
 *new_line_handle handles new line while reading script
 */
bool_t new_line_handle(unsigned int* n_newline, stack_t opp_stack,
    stack_t command_stack, command_stream_t m_command_stream){
  if (*n_newline == 1){
    stack_push(opp_stack, ";");
  }
  else if (*n_newline > 1){ 
    //form a command tree from what is currently in command stack and command
    //stream
    while (!stack_empty(opp_stack)){
      if (opp_pop_from_stack(command_stack, opp_stack) ==  FALSE)
        return FALSE;
    }
    command_t finished_command_tree;
    stack_pop(command_stack, &finished_command_tree);
    command_stream_add(m_command_stream, &finished_command_tree);
    //TODO: maybe some error handling here
  }

  *n_newline = 0; 
  return TRUE; 
}
//prechecks all things operator related
bool_t handle_opperators(stack_t command_stack, bool_t opp_bool){
  return (opp_bool || stack_empty(command_stack)) ? FALSE : TRUE;
}




command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  unsigned int paren_count = 0;
  unsigned int new_line_count = 0;
  bool_t opp_bool = TRUE;
  string_t simple_buffer = malloc(sizeof(struct string));
  string_new(simple_buffer);
  char curr_byte;
  //command streams
  command_stream_t c_trees = checked_malloc(sizeof(struct command_stream));
  
  command_stream_new(c_trees);
  //opp and command stack
  stack_t command_stack = checked_malloc(sizeof(struct stack));
  stack_new(command_stack, sizeof(command_t));
  //will be c strings
  stack_t opp_stack = checked_malloc(sizeof(struct stack));
  stack_new(command_stack, sizeof(char*));

  while ((curr_byte = get_next_byte(get_next_byte_argument))>= 0){
    return NULL;
  }


  

  string_delete(simple_buffer);
  free(simple_buffer);
  stack_delete(command_stack);
  free(command_stack);
  stack_delete(opp_stack);
  free(opp_stack);
  
  //do we need to free a malloced command stream

  //error (1, 0, "command reading not yet implemented");

  return 0;
}

command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  error (1, 0, "command reading not yet implemented");
  return 0;
}
