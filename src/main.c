#include "common.h"
#include "minima.h"
#include "minima_array.h"
#include "minima_eval.h"

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

#if 0
  log_info("program returned %d\n", result);

  for (int i = 0; i < program->symbols.count; i++)
  {
    MiSymbol* symbol = &program->symbols.entry[i];
    if (symbol->type != MI_SYMBOL_FUNCTION)
      continue;
    log_info("Function %s, %d args\n", symbol->identifier.str, symbol->as.function.param_count);
  }

  for (int i = 0; i < program->symbols.count; i++)
  {
    MiSymbol* symbol = &program->symbols.entry[i];
    if (symbol->type != MI_SYMBOL_VARIABLE)
      continue;

    if (symbol->as.variable.value.type == MI_TYPE_BOOL)
    {
      log_info("Variable %s:bool = %s\n", symbol->identifier.str, ((int) symbol->as.variable.value.as.number_value) ? "true" : "false");
    }
    else if (symbol->as.variable.value.type == MI_TYPE_INT)
    {
      log_info("Variable %s:int = %d\n", symbol->identifier.str, (int) symbol->as.variable.value.as.number_value);
    }
    else if (symbol->as.variable.value.type == MI_TYPE_FLOAT)
    {
      log_info("Variable %s:float = %f\n", symbol->identifier.str, symbol->as.variable.value.as.number_value);
    }
    else if (symbol->as.variable.value.type == MI_TYPE_STRING)
    {
      log_info("Variable %s:string = %s\n", symbol->identifier.str, symbol->as.variable.value.as.string_value);
    }
    else
    {
      log_info("Variable %s: unknown\n", symbol->identifier.str);
    }
  }
#endif
  mi_ast_program_destroy(program->ast);
  free(buffer);
  return result;
}

int test_array()
{
  MiArray* array = mi_array_create(4);

  mi_array_add_int(array, 10);
  mi_array_add_float(array, 3.14f);
  mi_array_add_string(array, "Hello World");

  MiArray* sub_array = mi_array_create(2);
  mi_array_add_int(sub_array, 42);
  mi_array_add_string(sub_array, "Nested Array");
  mi_array_add_array(array, sub_array);

  mi_array_print(array);

  mi_array_destroy(array);
  return 0;
}

int main(int argc, char **argv)
{
  //UNUSED(argc);
  //UNUSED(argv);
  //return test_array();
  return test_language(argc, argv);
}

