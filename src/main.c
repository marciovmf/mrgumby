#include "common.h"
#include "minima.h"
#include "minima_eval.h"


MiValue function_print(int param_count, MiValue* parameters)
{
  if (param_count == 1)
  {
    MiValue* value = &parameters[0];
    if (value->type == MI_VAL_BOOL)
    {
      printf("%s\n",((int) value->as.number_value) ? "true" : "false");
    }
    else if (value->type == MI_VAL_INT)
    {
      printf("%d\n", (int) value->as.number_value);
    }
    else if (value->type == MI_VAL_FLOAT)
    {
      printf("%f\n", value->as.number_value);
    }
    else if (value->type == MI_VAL_STRING)
    {
      printf("%s\n", value->as.string_value);
    }
    else
    {
      log_info("Runtime value %llxn", value);
    }
  }

  return mi_runtime_value_create_void();
}


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

  // Add a function into the symbol table
  MiSymbol* s = &program->symbols.entry[program->symbols.count++];
  smallstr(&s->identifier, "print");
  s->type = MI_SYMBOL_FUNCTION;
  s->as.function.param_count = 1;
  s->as.function.function_ptr = function_print;
  s->as.function.parameters = (MiVariable*) malloc(sizeof(MiVariable));
  s->as.function.parameters[0].name = "msg";
  s->as.function.parameters[0].value.type = MI_VAL_ANY;
  s->as.function.parameters[0].scopeLevel = 0;

  int result = mi_program_run(program);
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

    if (symbol->as.variable.value.type == MI_VAL_BOOL)
    {
      log_info("Variable %s:bool = %s\n", symbol->identifier.str, ((int) symbol->as.variable.value.as.number_value) ? "true" : "false");
    }
    else if (symbol->as.variable.value.type == MI_VAL_INT)
    {
      log_info("Variable %s:int = %d\n", symbol->identifier.str, (int) symbol->as.variable.value.as.number_value);
    }
    else if (symbol->as.variable.value.type == MI_VAL_FLOAT)
    {
      log_info("Variable %s:float = %f\n", symbol->identifier.str, symbol->as.variable.value.as.number_value);
    }
    else if (symbol->as.variable.value.type == MI_VAL_STRING)
    {
      log_info("Variable %s:string = %s\n", symbol->identifier.str, symbol->as.variable.value.as.string_value);
    }
    else
    {
      log_info("Variable %s: unknown\n", symbol->identifier.str);
    }
  }

  mi_ast_program_destroy(program->ast);
  free(buffer);
  return result;
}


int main(int argc, char **argv)
{
  return test_language(argc, argv);
}

