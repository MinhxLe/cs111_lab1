// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <string.h>
#include <stdlib.h>
/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */
//command node for linked list


//////////STACK IMPLEMENTATION///////////////////
typedef struct stack* stack_t;
struct stack{
  unsigned int count;
  unsigned int MAX_COUNT;
  //array of pointers pointing to corresponding data member
  void** base_ptr;
};

void stack_new(stack_t s){
  static const unsigned int STACK_INIT_SIZE = 8; 
  s = (stack_t)malloc(sizeof(struct stack));
  s->count = 0;
  s->MAX_COUNT = STACK_INIT_SIZE;
  s->base_ptr = malloc(sizeof(void*) * STACK_INIT_SIZE);
}

void stack_delete(stack_t s){
  for (unsigned int x = 0; x < s->count; x++){
    free(s->base_ptr[x]);
  }
  free(s->base_ptr);
  free(s);
}

void* stack_pop(stack_t s){
  if (s->count == 0)
    return NULL;
  
  s->count--;
  //frees up space just for shits and giggle
  //if (s->count <= s->MAX_COUNT / 2){
  //  MAX_COUNT /= 2;
  //  a->base_ptr = realloc(a->base_ptr, sizeof(void*) * MAX_COUNT);
  //}
  return s->base_ptr[s->count]; //returns the pointer
}

void stack_push(stack_t s, void* item){
  //if run out of space, allocate more
  if (s->count == s->MAX_COUNT){
    s->MAX_COUNT *= 2;
    s->base_ptr = realloc(s->base_ptr, sizeof(void*) * s->MAX_COUNT);
  }
  
  s->base_ptr[s->count] = item;
  s->count++;
}

//////COMMAND CONSTRUCTOR///////////
void command_new(command_t m_command, enum command_type t, int stat, 
    char* in, char* out, void * args, void * args[2]){
  
  //TODO: check what type,status, and ioredirection is
  m_command = (command_t)malloc(sizeof(struct command));
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
      m_command->u.command = (command_t*)args;
      break;
    case (SIMPLE_COMMAND):
      m_command->u.word = (char**)args;
      break;
    case (SUBSHELL_COMMAND):
      m_command->u.subshell_command = (command_t)args;
  }
}



///////COMMAND STREAM IMPLEMENTATION/////////////////////

//command stream implementation
struct command_stream{
  //points to list of ROOTS of command tree
  command_t* m_commands;
  unsigned int n_commands;
  unsigned int N_MAX_COMMANDS;
};

//initalizes new command_stream
void command_stream_new(command_stream_t m_command_stream){
  //creating initial command_stream
  m_command_stream = (command_stream_t)malloc(sizeof(struct command_stream));
  m_command_stream->n_commands = 0;
  m_command_stream->N_MAX_COMMANDS = 256; 
  m_command_stream->m_commands = (command_t*)malloc(sizeof(command_t) * m_command_stream->N_MAX_COMMANDS);
}

void command_stream_delete(command_stream_t cs){
  for (unsigned int x = 0; x < cs->n_commands;x++)
    free(cs->n_commands[x]);
  free(cs->m_commands);
  free(cs);
}

void command_stream_add(command_stream_t s, command_t c){
  if (s->n_commands == s->N_MAX_COMMANDS){
    s->N_MAX_COMMANDS *= 2;
    s->m_commands = (command_t*)realloc(s->m_commands, sizeof(command_t) * s->N_MAX_COMMANDS);
  }
  s->m_commands[s->n_commands] = c;
  s->n_commands++;

}
/*
///////HELPER FUNCTIONS//////////

//TODO FINISH THIS
int find_substr(char* str, char* targ, unsigned int start, unsigned int end){
  for (unsigned int x = start; x < end; x++){
    
  }
}

inline int min(int x, int y){
  if (x <= y)
    return x;
  else
    return y;
}

//////PARSING FUNCTION//////////
command_t ct_parse_seq(char* str, unsigned int start, unsigned int end){
  int first = find_substr(str, "\n", start,end);
  int second = find_substr(str, ";", start, end);
  int pos;
  if (first >= 0 && second >= 0)
    pos = min(first,second);
  else if (first >= 0)
    pos = first;
  else if (second >= 0)
    pos = second;
  else
    return NULL;
  


}

command_t ct_parse_andor(char* str, unsigned int start, unsigned int end){ 
  int first = find_substr(str, "||", start,end);
  int second = find_substr(str, "&&", start, end);
  int pos;
  if (first >= 0 && second >= 0)
    pos = min(first,second);
  else if (first >= 0)
    pos = first;
  else if (second >= 0)
    pos = second;
  else
    return NULL;
  

}

command_t ct_parse_pipe(char* str, unsigned int start, unsigned int end){
int pos = find_substr(str, "|", start,end);
if (pos < 0)
  return NULL;

}
command_t ct_parse_simple

*/



/*command_tree_parse
 * takes in a cstring limited by start and end(exclusive)
 * it also takes in a pointer to int line_err which it will 
 * change if there is an error in its current line number
 * (by counting white numbers)
 */
/*
command_t command_tree_parse(char* string, unsigned int start, unsigned int end, 
  unsigned int* line_err){
  //handles new line first



  else script_buff(opp_pos = find_substr(string, ";", start, end)) >= 0){
  }

  //hanlding
  else if ((opp_pos = find_substr(string, ";", start, end)) >= 0){
  }

  else if ((opp_pos = find_substr(string, ";", start, end)) >= 0){
  }
  else if ((opp_pos = find_substr(string, ";", start, end)) >= 0){
  }

  //handles new line first
  //then handles && or ||
  //then handls | 
  //
  //then handles ( )
  //then handles substr and #
  return NULL;
  //handle sequence commands(search for
  //handle 
}

*/
command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  

  int curr_byte;
  unsigned int BUFF_SIZE = 256;
  unsigned int curr_buff_size = 0;
  char* command_buff = (char*)malloc(sizeof(char) * BUFF_SIZE);


  command_stream_t m_command_stream = command_stream_new();
  //now parsing script
  stack_t command_stack = stack_new(); //stack of commands
  stack_t opperand_stack = stack_new(); // represents opperand of strings
  unsigned int line_num = 1;

  //reading in script to a buffer...one character at a time
  while ((curr_byte = get_next_byte(get_next_byte_argument)) >= 0){
     if  
    
    
    
    
    if (curr_buff_size == BUFF_SIZE){
      BUFF_SIZE *=2;
      script_buff = (char*) realloc(script_buff, BUFF_SIZE);
    }
    script_buff[curr_buff_size] = curr_byte;
    curr_buff_size++;
  }
  

  //TODO: handling comments before anything
  //go through the original string, finding consecutive new lines
  //once you find/reach the end
  //call command_tree_parse on that substring, adding the command_t pointer it
  //returns the root command_t



    /*
    if curr byte is a valid character
      append to current simple command
    else 
      push simple command(if valid) onto command stack
      switch statement for &, \n, ;, | , |, () needs to confirm next char make sense as well
      if 2+ new lines, break and repeat for next command tree
      

      
      push the link to the next comman
    */
  
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  
  error (1, 0, "command reading not yet implemented");

  return 0;
}

command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  error (1, 0, "command reading not yet implemented");
  return 0;
}
