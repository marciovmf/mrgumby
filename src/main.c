#include "common.h"
#include "ast.h"
#include "eval.h"
#include "parser.h"

#ifdef _WIN32
#define strdup _strdup
#endif


int main_ast(int argc, char** argv)
{
  UNUSED(argc);
  UNUSED(argv);

  // Initialize the symbol table
  SymbolTable table;
  symbol_table_init(&table);

  // Create the AST for the expression "a = 2 + 3 * 5"
  ASTExpression* literal_2 = ast_create_expression_literal_int(2);
  ASTExpression* literal_3 = ast_create_expression_literal_int(3);
  ASTExpression* literal_5 = ast_create_expression_literal_int(5);
  ASTExpression* multiplication = ast_create_expression_binary(literal_2, OP_ADD, literal_3);
  ASTExpression* addition = ast_create_expression_binary(multiplication, OP_MULTIPLY, literal_5);

  // assignment
  ASTStatement* assignment = ast_create_statement_assignment("a", addition);
  ASTStatementList* stmt_list = ast_create_statement_list(2);
  ASTStatement* print_statement = ast_create_statement_print(ast_create_expression_lvalue("a"));
  ast_statement_list_add(stmt_list, assignment);
  ast_statement_list_add(stmt_list, print_statement);

  ASTProgram* program = ast_create_program(stmt_list);

  // Evaluate the program
  int exit_code = eval_program(&table, program);
  ast_destroy_program(program);
  printf("Program return code: %d\n", (int) exit_code);
  return 0;
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
  int result = program == NULL ? 1 : 0;
  ast_destroy_program(program);
  free(buffer);
  return result;
}

int main(int argc, char **argv)
{
  return main_parser(argc, argv);
}

