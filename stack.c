#include "stack.h"
////////////////////////////////////////////
////////   STACK IMPLEMENTATION    /////////
////////////////////////////////////////////
#include <stdlib.h>
#include <stdio.h>
#include "alloc.h"
void stack_new(stack_t s, size_t element_size){
  s->n_elements = 0;
  s->v_stack = (vector_t)checked_malloc(sizeof(struct vector));
  vector_new(s->v_stack, element_size);
}



void stack_delete(stack_t s){
  vector_delete(s->v_stack);
  free(s->v_stack);
}

bool_t stack_pop(stack_t s, void* dest){
  if (s->n_elements == 0)
    return FALSE;
  
  s->n_elements--;
  vector_get(s->v_stack, s->n_elements, dest); //returns the pointer
  vector_remove(s->v_stack, s->n_elements); 
  return TRUE;
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
  return (s->n_elements == 0);
}


