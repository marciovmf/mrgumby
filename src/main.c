#include "common.h"
#include "minima.h"
#include <stdio.h>
#include <stdlib.h>

int test_language(int argc, char **argv)
{
  char *buffer;
  size_t fileSize;

  if (argc > 1)
  {
    if (read_entire_file_to_memory(argv[1], &buffer, &fileSize, true) != 0)
    {
      return 1; 
    }
  } else
  {
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    return 1;
  }

  MiProgram* program = mi_program_create(buffer);


  int result = mi_program_run(program);

  mi_ast_program_destroy(program->ast);
  free(buffer);
  return result;
}

int main(int argc, char **argv)
{
  return test_language(argc, argv);
}
