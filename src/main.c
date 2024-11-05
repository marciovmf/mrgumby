#include "common.h"
#include "parser.h"
#include "ast.h"
#include "eval.h"


RTValue function_print(int param_count, RTValue* parameters)
{
  if (param_count == 1)
  {
    RTValue* value = &parameters[0];
    if (value->type == RT_VAL_BOOL)
    {
      printf("%s\n",((int) value->as.number_value) ? "true" : "false");
    }
    else if (value->type == RT_VAL_INT)
    {
      printf("%d\n", (int) value->as.number_value);
    }
    else if (value->type == RT_VAL_FLOAT)
    {
      printf("%f\n", value->as.number_value);
    }
    else if (value->type == RT_VAL_STRING)
    {
      printf("%s\n", value->as.string_value);
    }
    else
    {
      log_info("Runtime value %llxn", value);
    }
  }

  return runtime_value_create_void();
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

  ASTProgram* program = parse_program(buffer);

  RTSymbolTable symbol_table;
  symbol_table_init(&symbol_table);

  // Add a function into the symbol table
  RTSymbol* s = &symbol_table.entry[symbol_table.count++];
  smallstr(&s->identifier, "print");
  s->type = RT_SYMBOL_FUNCTION;
  s->as.function.param_count = 1;
  s->as.function.function_ptr = function_print;
  s->as.function.parameters = (RTVariable*) malloc(sizeof(RTVariable));
  s->as.function.parameters[0].name = "msg";
  s->as.function.parameters[0].value.type = RT_VAL_ANY;
  s->as.function.parameters[0].scopeLevel = 0;

  int result = eval_program(&symbol_table, program);
  log_info("program returned %d\n", result);

  for (int i = 0; i < symbol_table.count; i++)
  {
    RTSymbol* symbol = &symbol_table.entry[i];
    if (symbol->type != RT_SYMBOL_FUNCTION)
      continue;
    log_info("Function %s, %d args\n", symbol->identifier.str, symbol->as.function.param_count);
  }

  for (int i = 0; i < symbol_table.count; i++)
  {
    RTSymbol* symbol = &symbol_table.entry[i];
    if (symbol->type != RT_SYMBOL_VARIABLE)
      continue;

    if (symbol->as.variable.value.type == EXPR_LITERAL_BOOL)
    {
      log_info("Variable %s:bool = %s\n", symbol->identifier.str, ((int) symbol->as.variable.value.as.number_value) ? "true" : "false");
    }
    else if (symbol->as.variable.value.type == EXPR_LITERAL_INT)
    {
      log_info("Variable %s:int = %d\n", symbol->identifier.str, (int) symbol->as.variable.value.as.number_value);
    }
    else if (symbol->as.variable.value.type == EXPR_LITERAL_FLOAT)
    {
      log_info("Variable %s:float = %f\n", symbol->identifier.str, symbol->as.variable.value.as.number_value);
    }
    else if (symbol->as.variable.value.type == EXPR_LITERAL_STRING)
    {
      log_info("Variable %s:string = %s\n", symbol->identifier.str, symbol->as.variable.value.as.string_value);
    }
    else {
      log_info("Variable %s: unknown\n", symbol->identifier.str);
    }
  }

  ast_destroy_program(program);
  free(buffer);
  return result;
}

int main(int argc, char **argv)
{
  return test_language(argc, argv);
}

