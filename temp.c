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
    case OUTPUT:
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
  vector_new(m_command_stream->command_trees, sizeof(command_t));
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

//prechecks all things operator related
bool_t operator_check(stack_t command_stack, bool_t opp_bool){
  return !(opp_bool || stack_empty(command_stack));
}

// checks to see if a char is an operator
bool_t is_operator(char c) {
  return (c == '(' || c == ')' || c == '\n' || c == '#'
    || c == ';' || c == '|' || c == '&' || c == '>'
    || c == '<');
}

bool_t is_command_char(char c){
  return ('0' <= c && c <= '9') || ('A' <= c && c<= 'Z') || ('a' <= c && c <= 'z') || 
    c == '!' || c == '%' || c == ',' || c == '-' || c == '/' ||
    c == '.' || c == ':' || c == '@' || c == '^' || c == '_';
}

bool_t is_whitespace(char c) {
  return c == ' ' || c == '\t';
}

// returns 1 if last item was an operator
// returns 0 if last item was creating a new command tree
// returns -1 if nothing was changed
int handle_newlines(unsigned int* new_line_count, string_t command_string,) {
  if (new_line_count == !) {
    string_append_char(command_string, ';');
    return 1;
  }
  else if (new_line_count > 0) {
    string_append_char(command_string, '\0');
    return 0;
  }

  new_line_count = 0;
  return -1;
}

string_t check_syntax (string_t command_string) {
  string_t new_command_string = checked_malloc(sizeof(struct string));
  string_new(new_command_string);

  unsigned int paren_count = 0; //counts paren
  unsigned int new_line_count = 0; //new line count
  unsigned int line_number = 1;
  bool_t opp_bool = TRUE; //there was an operator before

  // this isn't corect syntax for accesing character in string but let's pretend it is
  for (int i = 0; command_string[i] != EOF; i++) {
    int handle_newline = handle_newlines(new_line_count, new_command_string);
    if (handle_newline != -1)
      opp_bool = handle_newline;

    if (command_string[i] == '(') {
      if (!opp_bool) {
          fprintf(stderr, "%d: No operator before ( \n", line_number);
          exit(0);
      }

      string_append_char(new_command_string, '(');
      opp_bool = TRUE;
      paren_count++;
    }
    else if (command_string[i] == ')') {
      if (opp_bool) {
        fprintf(stderr, "%d: Char before ) is operator \n", line_number);
        exit(0);
      }

      string_append_char(new_command_string, ')');
      opp_bool = FALSE;
      paren_count--;
    }
    else if (command_string[i] == '\n') {
      if (opp_bool || paren_count > 0)
        continue;
      new_line_count++;
    }
    else if (command_string[i] == ';') {
      if (opp_bool) {
        fprintf(stderr, "%d: Extra operator before ; \n", line_number);
        exit(0);
      }

      string_append_char(new_command_string, ';');
      opp_bool = TRUE;
    }
    else if (command_string[i] == '|') {
      if (opp_bool) {
        fprintf(stderr, "%d: Extra operator before | \n", line_number);
        exit(0);
      }
      string_append_char(new_command_string);

      if (command_string[i+1] == '|') {
        string_append_char(new_command_string, '|');
        i++;
      }

      opp_bool = TRUE;
    }
    else if (command_string[i] == '&') {
      if (opp_bool) {
        fprintf(stderr, "%d: Extra operator before & \n", line_number);
        exit(0);
      }

      if (command_string[i+1] != '&') {
        fprintf(stderr, "%d: No & following the first & \n", line_number);
        exit(0);
      }

      string_append(new_command_string, "&&");
      opp_bool = TRUE;
    }
    else if (command_string[i] == '>') {
      if (opp_bool) {
        fprintf(stderr, "%d: Extra operator before > \n", line_number);
        exit(0);
      }

      string_append_char(new_command_string, '>');
      opp_bool = TRUE;
    }
    else if (command_string[i] == '<') {
      if (opp_bool) {
        fprintf(stderr, "%d: Extra operator before < \n", line_number);
        exit(0);
      }

      string_append_char(new_command_string, '<');
      opp_bool = TRUE;
    }
    else { // default characters
      if (opp_bool) {
        while (!is_command_char(command_string[i])) {
          i++;
        }
      }
      
      
    }
  }

  command_string = new_command_string;
  // free new_command_tree
  return command_string;
}

  command_stream_t
make_command_stream (int (*get_next_byte) (void *),
    void *get_next_byte_argument)
{
  string_t command_string = checked_malloc(sizeof(struct string));
  string_new(command_string);
  char c;
  
  for (;;) {
    c = get_next_byte(get_next_byte_argument);
    if (c == EOF) break;

    if (c != '#')
      string_append_char(command_string, c);
    else { // remove comments
      while (c != '\n' && c != EOF) {
        c = get_next_byte(get_next_byte_argument);
      }

      if (c == EOF) {
        if (opp_bool) {
          fprintf(stderr, "%d: Ended with an operator\n", line_number);
          exit(0);
        }
        else if (paren_count > 0)
          fprintf(stderr, "%d: Ended with an open paren\n", line_number);
          exit(0);
      }
      else
        string_append_char(command_string, c);
    }
  }

  string_append_char(command_string, EOF);
  command_string = check_syntax(command_string, command_string_length);
}

void print_command_stream(command_stream_t t){
  //printf("%d", t->n_commands);
  command_t c;
  vector_get(t->command_trees, 0, &c);
  //print_command(c);
  //printf("%d", c->u.word+1);
   
  switch (c->type){
    
    //case SIMPLE_COMMAND:
    case AND_COMMAND:
      printf("this is an && statement");
      break;
    case SEQUENCE_COMMAND:
      printf("this is a sequence statement");
      break;
    case OR_COMMAND:
      printf("this is an || statement");
      break;
    case PIPE_COMMAND:
      printf("this is an | statement");
      break;
    case SUBSHELL_COMMAND:
      printf("YES");
      break;
    default:
      printf("NO");
      break;
     
  }
  
}



  command_t
read_command_stream (command_stream_t s)
{

  /* FIXME: Replace this with your implementation too.  */
  error (1, 0, "command reading not yet implemented");
  return 0;
}
