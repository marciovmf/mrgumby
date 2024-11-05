#include "common.h"
#include "ast.h"
#include "eval.h"
#include "parser.h"

#ifdef _WIN32
#define strdup _strdup
#endif

ExpressionValue function_print(int num_args, ExpressionValue* types)
{
  if (num_args == 1)
  {
    ExpressionValue* value = &types[0];
    if (value->type == EXPR_LITERAL_BOOL)
    {
      log_info("%s\n",((int) value->as.number_value) ? "true" : "false");
    }
    else if (value->type == EXPR_LITERAL_INT)
    {
      log_info("%d\n", (int) value->as.number_value);
    }
    else if (value->type == EXPR_LITERAL_FLOAT)
    {
      log_info("%f\n", value->as.number_value);
    }
    else if (value->type == EXPR_LITERAL_STRING)
    {
      log_info("%s\n", value->as.string_value);
    }
    else
    {
      log_info("Runtime value %llxn", value);
    }
  }

  return runtime_value_create_void();
}


int main_parser(int argc, char **argv)
{
  Lexer lexer;
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

  lexer_init(&lexer, buffer);
  ASTProgram* program = parse_program(&lexer);

  SymbolTable symbol_table;
  symbol_table_init(&symbol_table);

  // Add a function into the symbol table
  Symbol* s = &symbol_table.entry[symbol_table.count++];
  smallstr(&s->identifier, "print");
  s->type = SYMBOL_FUNCTION;
  s->as.function.num_args = 1;
  s->as.function.args[0] = EXPR_LITERAL_INT;
  s->as.function.function_ptr = function_print;

  int result = eval_program(&symbol_table, program);
  log_info("program returned %d\n", result);

  for (int i = 0; i < symbol_table.count; i++)
  {
    Symbol* symbol = &symbol_table.entry[i];
    if (symbol->type != SYMBOL_FUNCTION)
      continue;
    log_info("Function %s, %d args\n", symbol->identifier.str, symbol->as.function.num_args);
  }

  for (int i = 0; i < symbol_table.count; i++)
  {
    Symbol* symbol = &symbol_table.entry[i];
    if (symbol->type != SYMBOL_VARAIBLE)
      continue;

    if (symbol->as.variable.type == EXPR_LITERAL_BOOL)
    {
      log_info("Variable %s:bool = %s\n", symbol->identifier.str, ((int) symbol->as.variable.as.number_value) ? "true" : "false");
    }
    else if (symbol->as.variable.type == EXPR_LITERAL_INT)
    {
      log_info("Variable %s:int = %d\n", symbol->identifier.str, (int) symbol->as.variable.as.number_value);
    }
    else if (symbol->as.variable.type == EXPR_LITERAL_FLOAT)
    {
      log_info("Variable %s:float = %f\n", symbol->identifier.str, symbol->as.variable.as.number_value);
    }
    else if (symbol->as.variable.type == EXPR_LITERAL_STRING)
    {
      log_info("Variable %s:string = %s\n", symbol->identifier.str, symbol->as.variable.as.string_value);
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
  return main_parser(argc, argv);
}

