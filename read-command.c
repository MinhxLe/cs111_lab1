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
  command_t dest = checked_malloc(sizeof(struct command));
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

bool_t create_new_command_tree(stack_t command_stack, stack_t opp_stack,
    command_stream_t m_command_stream){
    while (!stack_empty(opp_stack)){
      if (opp_handle_stacks(command_stack, opp_stack) ==  FALSE)
        return FALSE;
    }
  //adding the command tree 
  command_t finished_command_tree;
  stack_pop(command_stack, &finished_command_tree);
  
  if (!stack_empty(command_stack))
    return FALSE;

  command_stream_add(m_command_stream, &finished_command_tree);
  return TRUE;
}


/*
 *opp_handle_new_lines handles new line while reading script
 */
bool_t opp_handle_new_lines(unsigned int* n_newline, stack_t opp_stack,
    stack_t command_stack, command_stream_t m_command_stream,  bool_t* opp_bool){
  if (*n_newline == 1){
    stack_push(opp_stack, ";");
    *opp_bool = TRUE;
  }
  //form a new command tree
  else if (*n_newline > 1){ 
    while (!stack_empty(opp_stack)){
      if (opp_handle_stacks(command_stack, opp_stack) ==  FALSE)
        return FALSE;
    }
    if (!create_new_command_tree(command_stack, opp_stack, m_command_stream))
      return FALSE;
  }
  *n_newline = 0; 

  return TRUE; 
}
//prechecks all things operator related
bool_t operator_check(stack_t command_stack, bool_t opp_bool){
  return !(opp_bool || stack_empty(command_stack));
}

// checks to see if a char is an operator
bool_t is_operator(char c) {
  return c != '(' && c != ')' && c != '\n' && c != '#'
    && c != ';' && c != '|' && c != '&' && c != '>'
    && c != '<';
}
bool_t is_command_char(char c){
  return ('0' <= c && c <= '9') || ('A' <= c && c<= 'Z') || ('a' <= c && c <= 'z') || 
    c == '!' || c == '%' || c == ',' || c == '-' || c == '/' ||
    c == '.' || c == ':' || c == '@' || c == '^' || c == '_';
}

bool_t handle_simple_commands(string_t buff, command_t* s, unsigned int wc){
  char** word = checked_malloc(sizeof (char*) * wc);
  *s = checked_malloc(sizeof (struct command));
  command_new (*s, SIMPLE_COMMAND, -1, NULL, NULL,(void*)&word);
  //printf("%d\n", word);
  //printf(*s->u.word);
  size_t start = 0;
  size_t end = 0;
  size_t word_count = 0;
  char c = '\0';
  //printf("%d\n", buff->length);
  for (size_t it = 0; it < buff->length; it++){
    string_get_char(buff, it, &c);
    //printf("%d\n", c);
    //printf("%d\n", buff->length);
    if (c == '\0'){
      string_to_new_cstring(buff, &word[word_count],start,end);//creating a new string
      //printf(word[word_count]);
      word_count++;
      start = end + 1;
      end = start;
      //printf("%d\n", start);
    }
    else
      end++;
  }
   
  //printf("%d\n", s);
  return TRUE;
}
//handles most* opperators
bool_t handle_operator(stack_t command_stack, stack_t opp_stack, char* opp){
  char* c = "NOT (";
  stack_top(opp_stack, c);

  while (!stack_empty(opp_stack) && strcmp(c, "(") != 0 && precedence(c, opp) > -1){
    opp_handle_stacks(command_stack, opp_stack);
    stack_top(opp_stack, c);
  }
  stack_push(opp_stack, &opp);
  return TRUE;
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

  string_t simple_buffer = checked_malloc(sizeof(struct string));
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
  stack_new(opp_stack, sizeof(char*));

  
  
  while (curr_byte != EOF){
    if (!checked_next)
      curr_byte = get_next_byte(get_next_byte_argument);
    checked_next = FALSE;
   
    if (curr_byte ==  '('){
      if (!opp_handle_new_lines(&new_line_count, opp_stack, command_stack, c_trees, &opp_bool))
      fprintf(stderr, "%d:FUCK YOU",line_number );
      stack_push(opp_stack, &"(");
      paren_count++;
      opp_bool = TRUE;
    }
    else if (curr_byte ==  ')'){
      //handling new line
      if (!opp_handle_new_lines(&new_line_count, opp_stack, command_stack, c_trees, &opp_bool))
        fprintf(stderr, "%dFUCK YOU", line_number);//TODO ERROR HERE
      if (stack_empty(opp_stack))
        fprintf(stderr, "%dFUCK YOU", line_number); // TODO
      char* temp;
      stack_top(opp_stack, temp);
      // NOTE: might be able to do this as a function (opp_handle_stacks but modified)
      while (strcmp(temp, "(") != 0){
        if (!opp_handle_stacks(command_stack, opp_stack))
          fprintf(stderr, "%dFUCK YOU", line_number);
        stack_top(opp_stack, temp);
      }

      //popping top off which is (
      stack_pop(opp_stack, &temp);
      paren_count--;
      opp_bool = FALSE;

      //the top of command stack will be the subshell command
      command_t new_subshell_command = checked_malloc(sizeof(struct command));
      command_t subshell_command;
      stack_pop(command_stack, &subshell_command);
      command_new(new_subshell_command, SUBSHELL_COMMAND, -1, NULL, NULL, (void*)&subshell_command);
      stack_push(command_stack, &new_subshell_command);
    }
    else if (curr_byte == '#'){
      //reading bytes until end of file or new line
      while ((curr_byte = get_next_byte(get_next_byte_argument))>= 0 && 
          curr_byte != '\n')
        continue;
      //since you attempted to read to EOF, you need to see if there are errors
      if (curr_byte < 0){
        if (paren_count > 0 || opp_bool)
          fprintf(stderr, "%dFUCK YOU", line_number);
      }
      checked_next = TRUE;
    }
    else if (curr_byte ==  '\n'){
      line_number++;
      if (paren_count > 0 || opp_bool)
        new_line_count++;
    }
    else if (curr_byte == ';'){
      //you want to keep popping stack and creating a subtree command since
      //this has the smallest precedence
      if (!operator_check(command_stack, opp_bool))
        fprintf(stderr, "%dFUCK YOU", line_number); 
      handle_operator(command_stack, opp_stack, ";");
    }
    else if (curr_byte ==  '|'){
      if (!operator_check(command_stack, opp_bool))
        fprintf(stderr, "%dFUCK YOU", line_number);
      if ((curr_byte = get_next_byte(get_next_byte_argument) >= 0 && curr_byte == '|')){
        handle_operator(command_stack, opp_stack, "||");  
      }
      else{
        if (curr_byte < 0)
          fprintf(stderr, "%dFUCK YOU", line_number);
        checked_next = TRUE;
        //handle_operator(command_stack, opp_stack, "|")
        //we don't need to call because | has highest precedence
        stack_push(opp_stack, &"|");

      } 
      opp_bool = TRUE;
    }
    else if (curr_byte == '&'){
      if (!operator_check(command_stack, opp_bool))
        fprintf(stderr, "%dFUCK YOU", line_number);
      if (!(curr_byte = get_next_byte(get_next_byte_argument) >= 0 && curr_byte == '&'))
        fprintf(stderr, "%dFUCK YOU", line_number);
      //don't need to checked_next because & comes in pairs
      handle_operator(command_stack, opp_stack, "&&");
    }
    else if (curr_byte == '>' || curr_byte == '<'){
      char current = curr_byte;
      //handling different types of IO redirects
      char curr_byte = get_next_byte(get_next_byte_argument);
      if (curr_byte >= 0) {
        if (curr_byte == '>' || curr_byte == '&')
          curr_byte = get_next_byte(get_next_byte_argument);
        else if (current == '>' && curr_byte == '|')
          curr_byte = get_next_byte(get_next_byte_argument);
      }
      else
        fprintf(stderr, "%dFUCK YOU", line_number);

      // remove extra whitespace
      while (curr_byte == ' ' || curr_byte == '\t') {
        curr_byte = get_next_byte(get_next_byte_argument);
      }
      //keep reading for file name
      while (is_command_char(curr_byte)) {
        string_append_char(simple_buffer, curr_byte);
        curr_byte = get_next_byte(get_next_byte_argument);
      }
      //string_append_char(simple_buffer, '\0');
      command_t com;
      stack_pop(command_stack, &com);
      //creating file out of buffer
      char* f = NULL;
      //pls
      string_to_new_cstring(simple_buffer, &f, 0, simple_buffer->length);
      if (current == '>')   
        command_set_io(com, f, OUTPUT);
      else
        command_set_io(com, f, INPUT);

      stack_push(command_stack, &com);
      string_clear(simple_buffer);
      checked_next = TRUE;
    }
  
    else{
      bool_t curr_whitespace = TRUE;
      unsigned int word_count = 0;
      //keep reading in character into simple buffer
      do{
        if (!curr_whitespace && (curr_byte == ' ' || curr_byte == '\t')){
          //push a null byte
          word_count++;
          string_append_char(simple_buffer, '\0');
          curr_whitespace = TRUE;
        }
        else {
          //printf("%c\n", curr_byte);
          curr_whitespace = FALSE; 
          if (is_command_char(curr_byte)){
            string_append_char(simple_buffer, curr_byte);
          } 
          else if (is_operator(curr_byte) || curr_byte == '\n'){
            word_count++;
            string_append_char(simple_buffer, '\0');
            checked_next = TRUE;
          }
          else
            fprintf(stderr, "%dFUCK YOU", line_number);  
        }
      }while ((curr_byte = get_next_byte(get_next_byte_argument))!= EOF);
      //adding simple command
      command_t simple;
      handle_simple_commands(simple_buffer,&simple,  word_count);
      //printf("%d\n", simple->u.word[1]);
      stack_push(command_stack, &simple);
      command_t test;
      stack_top(command_stack, &test);
    }

  } //while
  //CHECK IF THERE IS ANYTHING ON COMMAND STACK
  
  //NOT EXACTLY RIGHT
  if (!stack_empty(command_stack)){
    command_t test;
    stack_top(command_stack, &test);
    if(!create_new_command_tree(command_stack, opp_stack,c_trees))
        
        fprintf(stderr, "%dFUCK YOU", line_number);
  }




  string_delete(simple_buffer);
  free(simple_buffer);
  
  stack_delete(command_stack);
  free(command_stack);
  
  stack_delete(opp_stack);
  free(opp_stack);
  
  return c_trees;
}

void print_command_stream(command_stream_t t){
  //printf("%d", t->n_commands);
  command_t c;
  vector_get(t->command_trees, 0, &c);
  //print_command(c);
  printf(c->u.word[1]);
  /*
  switch (c->type){
    case SIMPLE_COMMAND:
    case AND_COMMAND:
    case SEQUENCE_COMMAND:
    case OR_COMMAND:
    case PIPE_COMMAND:
    case SUBSHELL_COMMAND:
      printf("YES");
      break;
    default:
      printf("NO");
      break;
     
  }
 */
}



  command_t
read_command_stream (command_stream_t s)
{

  /* FIXME: Replace this with your implementation too.  */
  error (1, 0, "command reading not yet implemented");
  return 0;
}
