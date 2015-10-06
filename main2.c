// UCLA CS 111 Lab 1 main program

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdio.h>

#include "command.h"
#include <stdlib.h>

static char const *program_name;
static char const *script_name;

static void
usage (void)
{
  error (1, 0, "usage: %s [-pt] SCRIPT-FILE", program_name);
}

static int
get_next_byte (void *stream)
{
  return getc (stream);
}

int
main ()
{
  /*
  printf("VECTOR TEST\n");
  vector_test();
  printf("STACK TEST\n");
  stack_test();
  
  printf("string Test\n");
  string_test();
  */
  
  script_name = "test.sh";
  FILE *script_stream = fopen (script_name, "r");
  if (! script_stream)
    error (1, errno, "%s: cannot open", script_name);
  command_stream_t command_stream = make_command_stream (get_next_byte, script_stream);
   
  //
  test_command_stream(command_stream);
  //printf("%d", command_stream->n_commands);
  //command_new(combined, AND_COMMAND, 0,0,0,(void*)c);

}
