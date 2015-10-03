#ifndef VECTOR_H
#define VECTOR_H
#include <stdlib.h>
#include "declarations.h"
typedef struct vector* vector_t;
struct vector{
  size_t ELEMENT_SIZE; // in bytes
  size_t n_elements;
  size_t N_MAX_ELEMENTS;
  void* elements;
};

void vector_new(vector_t s, size_t es);
void vector_delete(vector_t s);
void vector_set(vector_t s, size_t index, void* source);
void vector_get_elements(vector_t v, void* start);
bool_t vector_get(vector_t s, size_t index, void* dest);
bool_t vector_remove(vector_t s, size_t index);



void vector_clear(vector_t v);


#endif
