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
/*
 * Compares the precedence two operators
 * Returns 1  if precedence(opp1) > precedence(opp2)
 * Returns 0  if precedence(opp1) = precedence(opp2)
 * Returns -1 if precedence(opp1) < precedence(opp2)
 */
/*
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
*/
///////////////////////////////////////////////
//////////////READ COMMAND HELPERS/////////////
///////////////////////////////////////////////
/*
 *clean_raw_buffer takes in a string_t buffer(raw) not including /0
 *at the end preprocesses it to store on finished string
 *this includes syntax checking, removing comments, extra whitespaces, 
 *and tokenizes(?) it
 */
  void clean_raw_buffer (string_t raw_string, string_t finished_string) {

    //////necessary variables
    unsigned int paren_count = 0; //counts open parenthesis
    int line_number = 1; //keep tracks of current line number

    //////CONDITIONS/////////
    bool_t prev_oper = TRUE; //saw an operator before(true cause can't see it first)
    bool_t checked_next_char = FALSE;//checked next char
    bool_t handled_new_line = TRUE;//handled new line, used to ignore new lines after, first because you don't to handle it if it's first of the string
    bool_t at_comment = FALSE; //ignores until new line
    bool_t at_simple_command = FALSE;//currently at simple command
    bool_t at_space = TRUE; //true first because you don't want to handle it
    bool_t handled_io = TRUE; //case for handling IO
    //for every char in raw_buffer
    char curr_char;
    for (unsigned int i = 0; i < raw_string->length; i++ ){
      
      //if haven't read next char, read next char
      if (checked_next_char)
        i--;
      checked_next_char = FALSE;
      string_get_char(raw_string, i, &curr_char);

      //printf("%d\n", curr_char);
      if (curr_char == ' ' || curr_char == '\t'){
        if (at_simple_command){
          string_append_char(finished_string, '\0');
          at_simple_command = FALSE;
        }
        continue;
      }
      //comment 
      else if (curr_char == '#')
        at_comment = TRUE;
      //new line
      else if (curr_char == '\n'){
        if (at_simple_command){
          string_append_char(finished_string, '\0');//ending
          at_simple_command = FALSE;
        }
        if (!prev_oper && paren_count == 0 && !handled_new_line){
          //not last character
          //TODO IF It's \n ' ' \n
          if (string_get_char(raw_string, ++i, &curr_char) && curr_char == '\n'){
            string_append(finished_string, "\n\n");
          }
          else{
            string_append_char(finished_string, ';');
            prev_oper = TRUE;
            checked_next_char = TRUE;//even if you didn't check, it's okay 
          } 
        handled_new_line = TRUE; 
        }

        //handled_new_line = TRUE; 
        at_comment = FALSE;
        line_number++;
      }
      //not in comment
      else if (!at_comment){
        //true for all non new lines
        handled_new_line = FALSE;
        if (is_command_char(curr_char)){
          string_append_char(finished_string, curr_char);
          at_simple_command = TRUE;
          prev_oper= FALSE;
          handled_io = TRUE;
          
        }
        else if (curr_char == '<'){
          if (prev_oper || !handled_io) {
            fprintf(stderr, "%d: Consecutive operators (<)", line_number);
            exit(0);
          }
          if (!(string_get_char(raw_string, ++i, &curr_char) &&
                (curr_char == '>' || curr_char == '&')))
          checked_next_char = TRUE;
          string_append_char(finished_string, '<'); 
          handled_io = FALSE;
        }
        else if (curr_char == '>'){
          if (prev_oper || !handled_io) {
            fprintf(stderr, "%d: Consecutive operators (>)", line_number);
            exit(0);
          }
          if (!(string_get_char(raw_string, ++i, &curr_char) &&
                (curr_char == '>' || curr_char == '&' || curr_char == '|')))
            checked_next_char = TRUE;
          string_append_char(finished_string, '>'); 
          handled_io = FALSE;
        }

        //command operators
        else{
          if (at_simple_command){
            string_append_char(finished_string, '\0');//ending
            at_simple_command = FALSE;
          }
          //parentheses
          if (curr_char == '('){
            prev_oper = TRUE;
            paren_count ++;
            string_append_char(finished_string, '(');
          }
          else if (curr_char == ')'){
            if (paren_count > 0)
              paren_count--;
            else {
              fprintf(stderr, "%d: Close paren ) without a matching open paren (", line_number);
              exit(0);
            }
            if (prev_oper) {
              fprintf(stderr, "%d: Close paren ) following an operator", line_number);
              exit(0);//ERROR
            }
            prev_oper = FALSE;
            string_append_char(finished_string, ')');
          }

          //all other operators
          else if (is_operator(curr_char)){
            //true for all operators
            if (prev_oper) {
              fprintf(stderr, "%d: Consecutive operator", line_number);
              exit(0);//ERROR
            }
            prev_oper = TRUE;

            if (curr_char == ';'){

              string_append_char(finished_string, ';');
            }
            else if (curr_char == '&'){
              if (string_get_char(raw_string, ++i, &curr_char) && curr_char == '&')
                string_append(finished_string, "&&");
              else {
                fprintf(stderr, "%d: Char after & is not &", line_number);
                exit(0);
              }
            }
            else if (curr_char == '|'){
              //check if ||
              if (string_get_char(raw_string, ++i, &curr_char) && curr_char == '|'){
                string_append(finished_string, "||");
              }
              else{
                string_append_char(finished_string,'|'); 
                checked_next_char = TRUE;
              }
            }
          }
          //unhandled characters
          else {
            fprintf(stderr, "%d: Invalid character", line_number);
            exit(0);
          }
        }
      }
  }
      //FINAL CHECK AFTER GOING THROUGH ENTIRE STRING
      //if it ended with new lines, it should already be handled
      //paren count
      if (paren_count > 0){
        fprintf(stderr, "%d: extra (", line_number);
        exit(0);
      }
      if (!handled_io){ 
        fprintf(stderr, "%d: did not handle io", line_number);
        exit(0);
      }

      //extra ops (excluding ;)
      if (prev_oper){
        char prev;
        string_get_char(finished_string, finished_string->length - 1, &prev);
        //can end with a sequence operator
        if (prev == ';'){
          string_append(finished_string, "\n\n");
        }
        else{
        fprintf(stderr, "%d: extra operator", line_number);
        exit(0);
        }
      }
      if (at_simple_command) 
        string_append(finished_string, "\0\n\n");
  }

//SHOULD ALWAYS WORK BECAUSE YOUR INPUTED STRING IS VALID
void parse_command_tree(string_t string, command_stream_t tree){
  

}
  
  
  
  
  
  
  
  command_stream_t
make_command_stream (int (*get_next_byte) (void *),
    void *get_next_byte_argument)
{
  //reading every byte
  string_t raw_string = checked_malloc(sizeof(struct string));
  string_new(raw_string);
  char c;
  c = get_next_byte(get_next_byte_argument);
  while (c != EOF){
    string_append_char(raw_string, c);
    c = get_next_byte(get_next_byte_argument);
  }
  //cleaning up the string
  string_t clean_string = checked_malloc(sizeof(struct string));
  string_new(clean_string);
  //string_print(raw_string);
  clean_raw_buffer(raw_string, clean_string);
  string_print(clean_string);


  
  //freeing everything



}



  command_t
read_command_stream (command_stream_t s)
{

  /* FIXME: Replace this with your implementation too.  */
  error (1, 0, "command reading not yet implemented");
  return 0;
}
