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

#include "declarations.h"
#include "vector.h"
#include "stack.h"
#include "string.h"





////////////////////////////////////
//////COMMAND CONSTRUCTOR///////////
////////////////////////////////////


//creates a new command and stores it in the command_t inputted
void command_new(command_t m_command, enum command_type t, int stat, 
    char* in, char* out, void * args[2]){
  
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

//sets command io file
void command_set_io(command_t c, char* file, enum io r){
  switch(r){
    case INPUT:
      c->input = file;
    case OUPUT:
      c->output = file;
  }
}

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
  m_command_stream->command_trees = checked_malloc(sizeof (struct vector));
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
 * Compares the precedence two operators
 * Returns 1  if precedence(opp1) > precedence(opp2)
 * Returns 0  if precedence(opp1) = precedence(opp2)
 * Returns -1 if precedence(opp1) < precedence(opp2)
 */
int precedence(char* opp1, char* opp2) {
  if (strcmp(opp1, "|") == 0) {
    if (strcmp(opp2, "|") == 0)
      return 0;
    return 1; // | has the highest precedence
  }
  else if (strcmp(opp1, "&&") == 0 || strcmp(opp1, "||") == 0) {
    if (strcmp(opp2, "|") == 0)
      return -1;
    else if (strcmp(opp2, "&&") == 0 || strcmp(opp2, "||") == 0)
      return 0;
    return 1;
  }
  else { // opp1 = ;
    if (strcmp(opp2, ";") == 0)
      return 0;
    return -1;
  }
}

/*
 *pops 2 commnad from command stack, and creates a new command and
 *combines with popped stack, pushes them onto stack
 */
bool_t opp_handle_stacks(stack_t command_stack, stack_t opp_stack){
  //both of them shouldn't be empty?
  if (stack_empty(command_stack) || stack_empty(opp_stack))
    return FALSE;
  
  command_t commands[2];
  char* opperand;
  command_t dest = malloc(sizeof(struct command));
  stack_pop(command_stack, commands);
  stack_pop(command_stack, commands+1);
  stack_pop(opp_stack, &opperand); 
  
  // valid becaues you're just setting opperand to point to the popped cstring 
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

  //stack_push(command_stack, &dest);
  return TRUE;
}

/*
 *opp_handle_new_lines handles new line while reading script
 */
bool_t opp_handle_new_lines(unsigned int* n_newline, stack_t opp_stack,
    stack_t command_stack, command_stream_t m_command_stream){
  if (*n_newline == 1){
    stack_push(opp_stack, ";");
    opp_bool = TRUE;
  }
  //form a new command tree
  else if (*n_newline > 1){ 
    while (!stack_empty(opp_stack)){
      if (opp_handle_stacks(command_stack, opp_stack) ==  FALSE)
        return FALSE;
    }
    //adding the command tree 
    command_t finished_command_tree;
    stack_pop(command_stack, &finished_command_tree);
    command_stream_add(m_command_stream, &finished_command_tree);
  *n_newline = 0; 
  return TRUE; 
}
//prechecks all things operator related
bool_t handle_opperators(stack_t command_stack, bool_t opp_bool){
  return !(opp_bool || stack_empty(command_stack));
}

// checks to see if a char is an operator
bool_t is_operator(char c) {
  return c != '(' && c != ')' && c != '\n' && c != '#'
         && c != ';' && c != '|' && c != '&' && c != '>'
         && c != '<';
}


command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{

  unsigned int paren_count = 0; //counts paren
  unsigned int new_line_count = 0; //new line count
  unsigned int line_number = 1;
  bool_t opp_bool = TRUE; //there was an operator before
  bool_t checked_next = FALSE; //we checked the next character so don't read in another

  string_t simple_buffer = malloc(sizeof(struct string));
  string_new(simple_buffer);
  char curr_byte = 0;
  //command stream initialization
  command_stream_t c_trees = checked_malloc(sizeof(struct command_stream));
  command_stream_new(c_trees);

  //opp and command stack
  stack_t command_stack = checked_malloc(sizeof(struct stack));
  stack_new(command_stack, sizeof(command_t));
  //will be c strings
  stack_t opp_stack = checked_malloc(sizeof(struct stack));
  stack_new(command_stack, sizeof(char*));

  while (curr_byte >= 0){
    if (!checked_next)
      curr_byte = get_next_byte(get_next_byte_argument);
    checked_next = FALSE;
    switch(curr_byte){
      case '(':
        if (!opp_handle_new_lines(&new_line_count, opp_stack, command_stack, c_trees))
          return NULL;//TODO ERROR HERE
        stack_push(opp_stack, &"(");
        paren_count++;
        opp_bool = TRUE;
        break;
      case ')':
        //handling new line
        if (!opp_handle_new_lines(&new_line_count, opp_stack, command_stack, c_trees))
          return NULL;//TODO ERROR HERE
        if (stack_empty(opp_stack))
          return NULL; // TODO
        char* temp;
        stack_top(opp_stack, temp);
        // NOTE: might be able to do this as a function (opp_handle_stacks but modified)
        while (strcmp(temp, "(") != 0){
          if (!opp_handle_stacks(command_stack, opp_stack))
            return NULL;
          stack_top(opp_stack, temp);
        }

        //popping top off which is (
        stack_pop(opp_stack, &temp);
        paren_count--;
        opp_bool = FALSE;

        //the top of command stack will be the subshell command
        command_t new_subshell_command = malloc(sizeof(struct command));
        command_t subshell_command;
        stack_pop(command_stack, &subshell_command);
        command_new(new_subshell_command, SUBSHELL_COMMAND, -1, NULL, NULL, (void*)&subshell_command);
        stack_push(command_stack, &new_subshell_command);
        break;
      case '\n':
        line_number++;
        if (paren_count > 0 || opp_bool)
          break;
        new_line_count++;
        break;
      case '#':
        while ((curr_byte = get_next_byte(get_next_byte_argument))>= 0 && 
             curr_byte != '\n')
           continue;
        if (curr_byte == '\n') // if EOF then don't increment the line number
          line_number++;
        if (paren_count > 0 || opp_bool)
          break;
        new_line_count++;
        break;
      case ';':
        if (!handle_opperators(command_stack, opp_bool))
          return NULL; 
        char* c = "NOT (";
        stack_top(opp_stack, c);
        while (!stack_empty(opp_stack) && strcmp(c, "(") != 0 && precedence(c, ";") > -1){
          opp_handle_stacks(command_stack, opp_stack);
          stack_top(opp_stack, c);
        }
        stack_push(opp_stack, &";");

        opp_bool = TRUE;
        break;
      case '|':
        if (!handle_opperators(command_stack, opp_bool))
          return NULL;
        if ((curr_byte = get_next_byte(get_next_byte_argument) >= 0 && curr_byte == '|')){
          
          char* c = "NOT (";
          stack_top(opp_stack, c);
          while (!stack_empty(opp_stack) && strcmp(c, "(") != 0 && precedence(c, "||") > -1){
            opp_handle_stacks(command_stack, opp_stack);
            stack_top(opp_stack, c);
          }
          stack_push(opp_stack, &"||");
        }
        else{
          if (curr_byte < 0)
            return NULL;
          checked_next = TRUE;
          stack_push(opp_stack, &"|");
        
        } 
        opp_bool = TRUE;
        break;
      case '&':
        if (!handle_opperators(command_stack, opp_bool))
          return NULL;
        if (!(curr_byte = get_next_byte(get_next_byte_argument) >= 0 && curr_byte == '&'))
          return NULL;
        while (!stack_empty(opp_stack) && strcmp(c, "(") != 0 && precedence(c, "&&") > -1){
          opp_handle_stacks(command_stack, opp_stack);
          stack_top(opp_stack, c);
        }
        stack_push(opp_stack, &"&&");
        break;
      case '>': // aside from INPUT and OUTPUT for command_set_io and the fact that > accepts
                // '|' in addition to <'s '>' and '&' following the i/o redirect, '>' and '<'
                // are essentially the same
        curr_byte = get_next_byte(get_next_byte_argument);
        if (curr_byte >= 0 && (curr_byte == '>' || curr_byte == '&' || curr_byte == '|')) {
          curr_byte = get_next_byte(get_next_byte_argument);
        }
        else if (curr_byte < 0)
          return NULL;

        // remove extra whitespace
        while (curr_byte == ' ' || curr_byte == '\t') {
          curr_byte = get_next_byte(get_next_byte_argument);
        }

        while (!is_operator(curr_byte)) {
          string_append_char(simple_buffer, curr_byte);
          curr_byte = get_next_byte(get_next_byte_argument);
        }
        checked_next = TRUE;

        string_append_char(simple_buffer, '\0');
        
        command_t com;
        stack_pop(command_stack, &com);

        // TODO: finish setting the input of com
        // command_set_io(com, char* file, INPUT){

        stack_push(command_stack, &com);

        break;
      case '<':
        curr_byte = get_next_byte(get_next_byte_argument);
        if (curr_byte >= 0 && (curr_byte == '>' || curr_byte == '&')) {
          curr_byte = get_next_byte(get_next_byte_argument);
        }
        else if (curr_byte < 0)
          return NULL;

        // remove extra whitespace
        while (curr_byte == ' ' || curr_byte == '\t') {
          curr_byte = get_next_byte(get_next_byte_argument);
        }

        while (!is_operator(curr_byte)) {
          string_append_char(simple_buffer, curr_byte);
          curr_byte = get_next_byte(get_next_byte_argument);
        }
        checked_next = TRUE;

        string_append_char(simple_buffer, '\0');
        
        command_t com;
        stack_pop(command_stack, &com);

        // TODO: finish setting the input of com
        // command_set_io(com, char* file, OUTPUT){

        stack_push(command_stack, &com);

        break;
      default:
        if (opp_bool){ 
          new_line_count = 0;
          if (curr_byte == ' ' || curr_byte == '\t')
            break;
        }

        else{
          opp_handle_new_lines(&new_line_count, opp_stack, command_stack, c_trees); 
        }

        if (curr_byte == ' ' || curr_byte == '\t'){
          if (string_empty(simple_buffer))
            break;
          else{ 
            string_append_char(simple_buffer, '\0');
          }
        }
        else{
            string_append_char(simple_buffer, curr_byte);
            opp_bool = FALSE;
        }
        break;
      } //switch
    } //while
  
  

  string_delete(simple_buffer);
  free(simple_buffer);
  stack_delete(command_stack);
  free(command_stack);
  stack_delete(opp_stack);
  free(opp_stack);
  
  return 0;
}

command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  error (1, 0, "command reading not yet implemented");
  return 0;
}
