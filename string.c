#include "string.h"
#include "alloc.h"
#include <string.h>
////////////////////////////////////////////
////////   STRING IMPLEMENTATION    ////////
////////////////////////////////////////////

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

bool_t string_empty(string_t s){
  return s->length == 0;
}
void string_clear(string_t s){
  s->length = 0;

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

