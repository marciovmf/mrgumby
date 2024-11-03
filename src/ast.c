#include "common.h"
#include "ast.h"
#include <stdlib.h>
#include <string.h>

#define strdup strdup_safe

#define AST_CREATE_NODE(T) (T*)(malloc(sizeof(T)))
#define EXPRESSION_IS_LITERAL(e) ((e) != NULL && ((e)->type == EXPR_FACTOR || (e)->type == EXPR_UNARY || (e)->type == EXPR_LITERAL_INT || (e)->type == EXPR_LITERAL_FLOAT || (e)->type == EXPR_LITERAL_STRING))

void ast_destroy_expression(ASTExpression* expression)
{
  if (expression == NULL)
    return;

  switch (expression->type)
  {
  case EXPR_VOID:
    break;
  case EXPR_UNARY:
    free((void*) expression->as.unary_expr.expression);
    break;
  case EXPR_COMPARISON:
    free((void*) expression->as.comparison_expr.left);
    free((void*) expression->as.comparison_expr.right);
    break;
  case EXPR_LOGICAL:
    free((void*) expression->as.logical_expr.left);
    free((void*) expression->as.logical_expr.right);
    break;
  case EXPR_FACTOR:
    free((void*) expression->as.factor_expr.left);
    free((void*) expression->as.factor_expr.right);
    break;
  case EXPR_TERM:
    free((void*) expression->as.term_expr.left);
    free((void*) expression->as.term_expr.right);
    break;
  case EXPR_LITERAL_STRING:
    free(expression->as.string_literal);
    break;
  case EXPR_FUNCTION_CALL:
    ast_destroy_expression(expression->as.func_call_expr.args);
    break;
  case EXPR_LITERAL_INT:
  case EXPR_LITERAL_FLOAT:
  case EXPR_LVALUE:
    break;
  }

  free(expression);
}


void ast_destroy_statement(ASTStatement* statement)
{
  if (statement == NULL)
    return;

  switch(statement->type)
  {
    case AST_STATEMENT_ASSIGNMENT:
      ast_destroy_expression(statement->as.assignment.expression);
      break;
    case AST_STATEMENT_IF:
      ast_destroy_expression(statement->as.if_stmt.condition);
      ast_destroy_statement(statement->as.if_stmt.if_branch);
      ast_destroy_statement(statement->as.if_stmt.else_branch);
      break;
    case AST_STATEMENT_FOR:
      ast_destroy_expression(statement->as.for_stmt.init->expression);
      ast_destroy_expression(statement->as.for_stmt.update->expression);
      ast_destroy_expression(statement->as.for_stmt.condition);
      break;
    case AST_STATEMENT_WHILE:
      ast_destroy_expression(statement->as.while_stmt.condition);
      ast_destroy_statement(statement->as.while_stmt.body);
      break;
    case AST_STATEMENT_RETURN:
      ast_destroy_expression(statement->as.return_expr);
      break;
    case AST_STATEMENT_FUNCTION_DECL:
      ast_destroy_statement_list(statement->as.function_decl.body);
      ast_destroy_statement_list(statement->as.function_decl.params);
      break;
    case AST_STATEMENT_FUNCTION_CALL:
      ast_destroy_expression_list(statement->as.func_call_expr);
      break;
    case AST_STATEMENT_BLOCK:
      ast_destroy_statement_list(statement->as.block_stmt);
      break;
    case AST_STATEMENT_PRINT:
      ast_destroy_expression(statement->as.print_expr);
      break;
    case AST_STATEMENT_INPUT: break;
    case AST_STATEMENT_BREAK: break;
  }

  free(statement);
}


void ast_destroy_statement_list(ASTStatement* list)
{
  ASTStatement* next = list;
  while (next != NULL)
  {
    ASTStatement* statement = next;
    next = statement->next;
    ast_destroy_statement(statement); 
  }
}


void ast_destroy_program(ASTProgram* program)
{
  if (program == NULL)
    return;

  ast_destroy_statement_list(program->body);
  free(program);
}


void ast_destroy_expression_list(ASTExpression* list)
{
  ASTExpression* next = list;
  while (next != NULL)
  {
    ASTExpression* expression = next;
    next = expression->next;
    ast_destroy_expression(expression); 
  }
}


//
// Helper functions for ASTExpression nodes
//

ASTExpression* ast_create_expression_term(ASTExpression* left, ASTTermOperator op, ASTExpression* right)
{
  ASSERT(left != NULL && (left->type == EXPR_TERM ||left->type == EXPR_FACTOR || EXPRESSION_IS_LITERAL(left)));
  ASSERT(right == NULL || (right->type == EXPR_TERM || right->type == EXPR_FACTOR || EXPRESSION_IS_LITERAL(right)));

  ASTExpression* expr = AST_CREATE_NODE(ASTExpression);
  expr->type = EXPR_TERM;
  expr->as.term_expr.left   = left;
  expr->as.term_expr.right  = right;
  expr->as.term_expr.op     = op;
  expr->next = NULL;
  return expr;
}

ASTExpression* ast_create_expression_factor(ASTExpression* left, ASTFactorOperator op, ASTExpression* right)
{
  ASSERT(left != NULL && (left->type == EXPR_TERM || EXPRESSION_IS_LITERAL(left)));
  ASSERT(right != NULL && (right->type == EXPR_TERM || EXPRESSION_IS_LITERAL(right)));

  ASTExpression* expr = AST_CREATE_NODE(ASTExpression);
  expr->type = EXPR_FACTOR;
  expr->as.factor_expr.left   = left;
  expr->as.factor_expr.right  = right;
  expr->as.factor_expr.op     = op;
  expr->next = NULL;
  return expr;
}

ASTExpression* ast_create_expression_unary(ASTUnaryOperator op, ASTExpression* expression) 
{
  ASSERT(expression != NULL && (expression->type == EXPR_TERM || EXPRESSION_IS_LITERAL(expression)));

  ASTExpression* expr = AST_CREATE_NODE(ASTExpression);
  expr->type = EXPR_UNARY;
  expr->as.unary_expr.op = op;
  expr->as.unary_expr.expression = expression;
  expr->next = NULL;
  return expr;
}

ASTExpression* ast_create_expression_logical(ASTExpression* left, ASTLogicalOperator op, ASTExpression* right) 
{
  ASSERT(left != NULL && (left->type == EXPR_TERM || EXPRESSION_IS_LITERAL(left)));
  ASSERT(right != NULL && (right->type == EXPR_TERM || EXPRESSION_IS_LITERAL(right)));
  ASTExpression* expr = AST_CREATE_NODE(ASTExpression);
  expr->type = EXPR_LOGICAL;
  expr->as.logical_expr.left = left;
  expr->as.logical_expr.op = op;
  expr->as.logical_expr.right = right;
  expr->next = NULL;
  return expr;
}

ASTExpression* ast_create_expression_comparison(ASTExpression* left, ASTComparisonOperator op, ASTExpression* right) 
{
  ASTExpression* expr = AST_CREATE_NODE(ASTExpression);
  expr->type = EXPR_COMPARISON;
  expr->as.comparison_expr.left = left;
  expr->as.comparison_expr.op = op;
  expr->as.comparison_expr.right = right;
  expr->next = NULL;
  return expr;
}

ASTExpression* ast_create_expression_literal_int(int value) 
{
  ASTExpression* expr = AST_CREATE_NODE(ASTExpression);
  expr->type = EXPR_LITERAL_INT;
  expr->as.number_literal = (double) value;
  expr->next = NULL;
  return expr;
}

ASTExpression* ast_create_expression_literal_float(double value) 
{
  ASTExpression* expr = AST_CREATE_NODE(ASTExpression);
  expr->type = EXPR_LITERAL_FLOAT;
  expr->as.number_literal = value;
  expr->next = NULL;
  return expr;
}

ASTExpression* ast_create_expression_literal_string(const char* value) 
{
  ASTExpression* expr = AST_CREATE_NODE(ASTExpression);
  expr->type = EXPR_LITERAL_STRING;
  expr->as.string_literal = strdup(value);
  expr->next = NULL;
  return expr;
}

ASTExpression* ast_create_expression_lvalue(const char* identifier) 
{
  ASTExpression* expr = AST_CREATE_NODE(ASTExpression);
  expr->type = EXPR_LVALUE;
  smallstr(&expr->as.identifier, identifier);
  expr->next = NULL;
  return expr;
}


ASTExpression* ast_create_expression_function_call(const char* identifier, ASTExpression* args)
{
  ASTExpression* expr = AST_CREATE_NODE(ASTExpression);
  expr->type = EXPR_FUNCTION_CALL;
  smallstr(&expr->as.func_call_expr.identifier, identifier);
  expr->as.func_call_expr.args = args;
  expr->next = NULL;
  return expr;
}



//
// Helper functions for ASTStatement nodes
//

ASTStatement* ast_create_statement_assignment(const char* identifier, ASTExpression* expression) 
{
  ASTStatement* stmt = AST_CREATE_NODE(ASTStatement);
  stmt->type = AST_STATEMENT_ASSIGNMENT;
  smallstr(&stmt->as.assignment.identifier, identifier);
  stmt->as.assignment.expression = expression;
  stmt->next = NULL;
  return stmt;
}

ASTStatement* ast_create_statement_if(ASTExpression* condition, ASTStatement* if_branch, ASTStatement* else_branch) 
{
  ASSERT(if_branch != NULL);
  if (else_branch)
    ASSERT(else_branch->type != AST_STATEMENT_BLOCK);

  ASTStatement* stmt = AST_CREATE_NODE(ASTStatement);
  stmt->type = AST_STATEMENT_IF;
  stmt->as.if_stmt.condition = condition;
  stmt->as.if_stmt.if_branch = if_branch;
  stmt->as.if_stmt.else_branch = else_branch;
  stmt->next = NULL;
  return stmt;
}

ASTStatement* ast_create_statement_for(ASTAssignment* init, ASTExpression* condition, ASTAssignment* update, ASTStatement* body) 
{
  ASTStatement* stmt = AST_CREATE_NODE(ASTStatement);
  stmt->type = AST_STATEMENT_FOR;
  stmt->as.for_stmt.init = init;
  stmt->as.for_stmt.condition = condition;
  stmt->as.for_stmt.update = update;
  stmt->as.for_stmt.body = body;
  stmt->next = NULL;
  return stmt;
}

ASTStatement* ast_create_statement_while(ASTExpression* condition, ASTStatement* body) 
{
  ASTStatement* stmt = AST_CREATE_NODE(ASTStatement);
  stmt->type = AST_STATEMENT_WHILE;
  stmt->as.while_stmt.condition = condition;
  stmt->as.while_stmt.body = body;
  stmt->next = NULL;
  return stmt;
}

ASTStatement* ast_create_statement_return(ASTExpression* expression) 
{
  ASTStatement* stmt = AST_CREATE_NODE(ASTStatement);
  stmt->type = AST_STATEMENT_RETURN;
  stmt->as.return_expr = expression;
  stmt->next = NULL;
  return stmt;
}

ASTStatement* ast_create_statement_function_decl(const char* identifier, ASTStatement* params, ASTStatement* body) 
{
  ASTStatement* stmt = AST_CREATE_NODE(ASTStatement);
  stmt->type = AST_STATEMENT_FUNCTION_DECL;
  stmt->as.function_decl.identifier = strdup(identifier);
  stmt->as.function_decl.params = params;
  stmt->as.function_decl.body = body;
  stmt->next = NULL;
  return stmt;
}

ASTStatement* ast_create_statement_print(ASTExpression* expression) 
{
  ASTStatement* stmt = AST_CREATE_NODE(ASTStatement);
  stmt->type = AST_STATEMENT_PRINT;
  stmt->as.print_expr = expression;
  stmt->next = NULL;
  return stmt;
}

ASTStatement* ast_create_statement_input(const char* identifier) 
{
  ASTStatement* stmt = AST_CREATE_NODE(ASTStatement);
  stmt->type = AST_STATEMENT_INPUT;
  stmt->as.input_expr = ast_create_expression_lvalue(identifier);
  stmt->next = NULL;
  return stmt;
}

ASTStatement* ast_create_statement_break(void)
{
  ASTStatement* stmt = AST_CREATE_NODE(ASTStatement);
  stmt->type = AST_STATEMENT_BREAK;
  stmt->next = NULL;
  return stmt;
}

ASTStatement* ast_create_statement_function_call(ASTExpression* func_call_expr)
{
  ASSERT(func_call_expr != NULL);
  ASSERT(func_call_expr->type == EXPR_FUNCTION_CALL);

  ASTStatement* stmt = AST_CREATE_NODE(ASTStatement);
  stmt->type = AST_STATEMENT_FUNCTION_CALL;
  stmt->as.func_call_expr = func_call_expr;
  stmt->next = NULL;
  return stmt;
}

//
// Helper function to create a new block
//

ASTProgram* ast_create_program(ASTStatement* body) 
{
  ASTProgram* program = AST_CREATE_NODE(ASTProgram);
  program->body = body;
  return program;
}

