#include "minima.h"
#include "minima_ast.h"
#include "minima_parser.h"

MiProgram* mi_program_create(const char* source)
{
  MiProgram* program = (MiProgram*) malloc(sizeof(MiProgram));
  mi_symbol_table_init(&program->symbols);
  program->ast = mi_parse_program(source);
  return program;
}

int mi_program_run(MiProgram* program)
{
  return mi_eval_program(&program->symbols, program->ast);
}

void mi_program_destroy(MiProgram* program)
{
  free(program);
}

