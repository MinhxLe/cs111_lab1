#include "vector.h"
#include <stdlib.h>
#include <string.h>
#include "alloc.h"

#include <stdio.h>

void
vector_new (vector_t s, size_t es)
{
  s->ELEMENT_SIZE = es;
  s->n_elements = 0;//not really important, just signifies theright most element that has been changed
  s->N_MAX_ELEMENTS = 8;
  s->elements = (void*)checked_malloc (es * s->N_MAX_ELEMENTS);
}

void
vector_delete (vector_t s)
{
  free (s->elements);
}


void
vector_set (vector_t s, size_t index, void* source)
{
  while (index >= s->N_MAX_ELEMENTS)
    {
      s->N_MAX_ELEMENTS *= 2; 
      s->elements = (void*)checked_realloc (s->elements, s->ELEMENT_SIZE * s->N_MAX_ELEMENTS);
    }
  
  void* offset = (void*) ((char*)s->elements + index*s->ELEMENT_SIZE); //converting to byte size for (char*)
  memcpy (offset, source, s->ELEMENT_SIZE);
  s->n_elements = index + 1;
}

void
vector_append(vector_t s, void* source){
    vector_set(s, s->n_elements, source);
}


bool_t
vector_get (vector_t s, size_t index, void* dest)
{
  if (index < s->n_elements)//not a very good check
    {
      void* offset = (void*) ((char*)s->elements + index*s->ELEMENT_SIZE); //converting to byte size for (char*):V
      memcpy (dest, offset, s->ELEMENT_SIZE);
      return TRUE;
    }
  return FALSE;
}

bool_t
vector_remove (vector_t s, size_t index)
{
  if (index >= s->n_elements)
    return FALSE;
  else if (index == s->N_MAX_ELEMENTS - 1)
    s->N_MAX_ELEMENTS -= 1;
  else
    {
      char* offset = ((char*)s->elements + index*s->ELEMENT_SIZE); //converting to byte size
      memcpy ((void*)&offset, (void*)(offset + s->ELEMENT_SIZE), (s->n_elements-1 - index)*s->ELEMENT_SIZE);  
      s->n_elements -= 1;    
    }
  return TRUE;
}

void
vector_get_elements (vector_t v, void* start)
{
  start = v->elements;
}

void
vector_test()
{
  vector_t v = malloc (sizeof (struct vector));
  vector_new (v, sizeof (int));
  for (int x = 0; x < 100; x++)
    vector_set (v, x, &x);

  int y;
  for (int x = 0; x < 100; x++)
    {
      vector_get (v, x, &y);
      printf ("%d\n", y);
    }
  vector_delete(v);
  free(v);
}
