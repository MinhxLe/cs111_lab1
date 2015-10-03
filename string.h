#ifndef STRING_H
#define STRING_H
#include "declarations.h"
#include "vector.h"
#include <stdlib.h>
typedef struct string* string_t;
struct string{
  size_t length;
  vector_t v_string;
};
void string_new(string_t s);
void string_new_cstring(string_t s, char* str);
void string_delete(string_t s);
void string_append(string_t s, char* str);
void string_append_char(string_t s, char ch);
void string_clear(string_t s);

bool_t string_empty(string_t s);

size_t string_length(string_t s);

bool_t string_get_char(string_t s, size_t index, char* c);
bool_t string_to_cstring(string_t s, char* dest, size_t start, size_t end);//end exclusive
bool_t string_to_new_cstring(string_t s, char* dest, size_t start, size_t end);
  //this one dynamically allocates space for you!


void cstring_to_string(string_t dest, char* source);




#endif
