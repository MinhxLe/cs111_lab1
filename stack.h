#ifndef STACK_H
#define STACK_H

#include "declarations.h"
#include "vector.h"
typedef struct stack* stack_t;
struct stack{
  vector_t v_stack;
  size_t n_elements;
};

void stack_new(stack_t s, size_t element_size);
void stack_delete(stack_t s);
bool_t stack_pop(stack_t s, void* dest);
void stack_push(stack_t s, void* item);
bool_t stack_top(stack_t s, void* dest);

bool_t stack_empty(stack_t s);


void stack_test();

#endif
