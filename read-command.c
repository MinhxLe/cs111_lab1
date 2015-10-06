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
void command_set_io(command_t c, char* file, char r){
  switch(r){
    case '<':
      c->input = file;
    case '>':
      c->output = file;
  }
}

/////////////////////////////////////////////////
///////COMMAND STREAM IMPLEMENTATION/////////////
/////////////////////////////////////////////////

//initalizes new command_stream
void command_stream_new(command_stream_t m_command_stream){
  //creating initial command_stream
  m_command_stream->n_commands = 0;
  m_command_stream->curr_com_index = 0;
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

int precedence(char opp1, char opp2) {
  if (opp1 == '|') {
    if (opp2 == '|')
      return 0;
    return 1; // | has the highest precedence
  }
  else if (opp1 == '&' || opp1 == 'o') {
    if (opp2 == '|')
      return -1;
    else if (opp2 == '&' || opp2 == 'o')
      return 0;
    return 1;
  }
  else { // opp1 = ;
    if (opp2 == ';')
      return 0;
    return -1;
  }
}


/*
 *pops 2 commnad from command stack, and creates a new command and
 *combines with popped stack, pushes them onto stack
 */
void opp_handle_stacks(stack_t command_stack, stack_t opp_stack){
  //both of them shouldn't be empty?
  command_t commands[2];
  char opperand;
  command_t dest = checked_malloc(sizeof(struct command));
  
  stack_pop(command_stack, commands+1);
  stack_pop(command_stack, commands);
  stack_pop(opp_stack, &opperand); 
  // valid becaues you're just setting opperand to point to the popped cstring 
  if (opperand == '&'){
    //TODO: what is status
    command_new(dest, AND_COMMAND, -1, NULL, NULL, (void*)commands);  
  }

  else if (opperand == 'o'){
    command_new(dest, OR_COMMAND, -1, NULL, NULL, (void*)commands);  
  }
  else if (opperand  == ';'){
    command_new(dest, SEQUENCE_COMMAND, -1, NULL, NULL, (void*)commands);  
  }
  else if (opperand == '|'){ 
    command_new(dest, PIPE_COMMAND, -1, NULL, NULL, (void*)commands);  
  }
  else{
    printf("YOU DUN GOOFED\n");
    free(dest);
  }
  stack_push(command_stack, &dest);
}

void opp_create_simple_command(string_t buff, command_t s, unsigned int wc){
  char** word = checked_malloc(sizeof (char*) * wc);
  command_new (s, SIMPLE_COMMAND, -1, NULL, NULL,(void*)&word);
  //printf("%d\n", word);
  //printf(*s->u.word);
  size_t start = 0;
  size_t end = 0;
  size_t word_count = 0;
  char c = '\0';
  for (size_t it = 0; it < buff->length; it++){
    string_get_char(buff, it, &c);
    if (c == '\0'){
      string_to_new_cstring(buff, &word[word_count],start,end);//creating a new string
      word_count++;
      start = end + 1;
      end = start;
    }
    else
      end++;
  } 
}


void opp_handle_operator(stack_t command_stack, stack_t opp_stack, char opp){
  char c;
  stack_top(opp_stack, &c);

  while (!stack_empty(opp_stack) && c != '(' && precedence(c, opp) > -1){
    opp_handle_stacks(command_stack, opp_stack);
    stack_top(opp_stack, &c);
  }
  stack_push(opp_stack, &opp);
}

void opp_add_new_command_tree(stack_t command_stack, stack_t opp_stack,
    command_stream_t m_command_stream){
    while (!stack_empty(opp_stack)){
      opp_handle_stacks(command_stack, opp_stack);
    }
  command_t finished_command_tree;
  stack_pop(command_stack, &finished_command_tree);
  command_stream_add(m_command_stream, &finished_command_tree);
  m_command_stream->n_commands++;
}

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
            string_append(finished_string, "\n\n", 2);
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
                string_append(finished_string, "&&", 2);
              else {
                fprintf(stderr, "%d: Char after & is not &", line_number);
                exit(0);
              }
            }
            else if (curr_char == '|'){
              //check if ||
              if (string_get_char(raw_string, ++i, &curr_char) && curr_char == '|'){
                string_append(finished_string, "||", 2);
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
          string_append(finished_string, "\n\n", 2);
        }
        else{
        fprintf(stderr, "%d: extra operator", line_number);
        exit(0);
        }
      }
      if (at_simple_command) 
        string_append(finished_string, "\0\n\n", 3);
  }

//SHOULD ALWAYS WORK BECAUSE YOUR INPUTED STRING IS VALID
void parse_command_tree(string_t cln_string, command_stream_t tree){
  //necessary data structures
  stack_t com_stack = checked_malloc(sizeof(struct stack));
  stack_t op_stack = checked_malloc(sizeof(struct stack));
  stack_new(com_stack, sizeof(command_t));//stack of command pointers
  stack_new(op_stack, sizeof(char));
  string_t simple_buff = checked_malloc(sizeof(struct string));//unnecssary
  string_new(simple_buff);
    
  //for every character
  char curr_char;
  for (unsigned int i = 0; i < cln_string->length; i++){
    
    
    string_get_char(cln_string,i, &curr_char);
    //creates a simple command out of whatever is read in
    if (is_command_char(curr_char) || curr_char == '\0'){
      unsigned word_count = 0;
      while (is_command_char(curr_char) || curr_char == '\0'){
        string_append_char(simple_buff, curr_char); 
        string_get_char(cln_string,++i, &curr_char);
        if (curr_char == '\0')
          word_count++;
      }
      if (i < cln_string->length -1)
        i--;
      command_t new_simple = checked_malloc(sizeof(struct command));
      opp_create_simple_command(simple_buff, new_simple, word_count);
      stack_push(com_stack, &new_simple);
      string_clear(simple_buff);
    }
    else{
      
      string_clear(simple_buff);//you're seeing something else that's not part of the original character anymore

      if (curr_char == '('){
        char c = '(';
        stack_push(op_stack, &c);
      }
      else if (curr_char == ')'){
        char temp;
        stack_top(op_stack, &temp);
        while (temp != '('){
          opp_handle_stacks(com_stack, op_stack);
          stack_top(op_stack, &temp);
        }
        stack_pop(op_stack, &temp);
        //creating a new subshell command
        command_t subshell = checked_malloc(sizeof(struct command));
        command_t inner_command;
        stack_pop(com_stack, &inner_command);
        command_new(subshell, SUBSHELL_COMMAND, -1, NULL, NULL, (void*)&inner_command); 
        stack_push(com_stack, &subshell);
      }
      else if (curr_char == '\n'){
        i++;//skip next char cause we know for sure it's  new line
        opp_add_new_command_tree(com_stack, op_stack, tree);      
      }
      else if (curr_char == '&'){
        i++; //skip next char for sure
        opp_handle_operator(com_stack, op_stack, '&' );
      }
      else if (curr_char == '|'){
        string_get_char(cln_string,++i, &curr_char);
        if (curr_char == '|'){
          opp_handle_operator(com_stack, op_stack, 'o');
        }
        else{
          opp_handle_operator(com_stack, op_stack, '|');
            i--;
        }
      }
      else if (curr_char == ';'){
        opp_handle_operator(com_stack, op_stack, ';');

      }
      else if (curr_char == '<' || curr_char == '>'){
        char temp = curr_char;
        while(curr_char != '\0'){//signifying end of file
          string_append_char(simple_buff, curr_char);
          string_get_char(cln_string, ++i, &curr_char);
        }
        //creating new string for file name
        char* file;
        string_to_new_cstring(simple_buff, &file, 0, simple_buff->length);

        command_t com;
        stack_pop(com_stack, &com);
        command_set_io(com, file, temp);
        stack_push(com_stack, &com);
        string_clear(simple_buff);//clearing since you used to read file name
      }
      else{
        printf("SHOULD NEVER OCCUR");
      }  
    }
  }//else for all non simple characters
  stack_delete(com_stack);
  stack_delete(op_stack);
  string_delete(simple_buff);

  free(com_stack);
  free(op_stack);
  free(simple_buff);
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
  string_print(clean_string);
  clean_raw_buffer(raw_string, clean_string);
  //creating a new command sream
  command_stream_t m_tree = checked_malloc(sizeof(struct command_stream));
  command_stream_new(m_tree);
  parse_command_tree(clean_string, m_tree); 
  //TODO freeing everything


  return m_tree;
}

void test_command_stream(command_stream_t t){
  printf("%d\n", t->n_commands);
}



  command_t
read_command_stream (command_stream_t s)
{
  if (s->curr_com_index < s->n_commands){ 
    command_t returning_command;
    vector_get(s->command_trees, s->curr_com_index, &returning_command);
    s->curr_com_index++;
    return returning_command;
  }
  else
      return NULL;
}

