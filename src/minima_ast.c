#include "common.h"
#include "minima_ast.h"
#include <stdlib.h>
#include <string.h>

#define strdup strdup_safe

#define MI_ASTCREATE_NODE(T) (T*)(malloc(sizeof(T)))
#define EXPRESSION_IS_LITERAL_OR_LVALUE(e) ((e) != NULL && ((e)->type == EXPR_FACTOR || (e)->type == EXPR_UNARY || (e)->type == EXPR_LITERAL_BOOL || (e)->type == EXPR_LITERAL_INT || (e)->type == EXPR_LITERAL_FLOAT || (e)->type == EXPR_LITERAL_STRING || (e)->type == EXPR_LVALUE))

void mi_ast_expression_destroy(ASTExpression* expression)
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
    mi_ast_expression_destroy(expression->as.func_call_expr.args);
    break;
  case EXPR_LITERAL_BOOL:
  case EXPR_LITERAL_INT:
  case EXPR_LITERAL_FLOAT:
  case EXPR_LVALUE:
    break;
  }

  free(expression);
}


void mi_ast_statement_destroy(ASTStatement* statement)
{
  if (statement == NULL)
    return;

  switch(statement->type)
  {
    case AST_STATEMENT_ASSIGNMENT:
      mi_ast_expression_destroy(statement->as.assignment.expression);
      break;
    case AST_STATEMENT_IF:
      mi_ast_expression_destroy(statement->as.if_stmt.condition);
      mi_ast_statement_destroy(statement->as.if_stmt.if_branch);
      mi_ast_statement_destroy(statement->as.if_stmt.else_branch);
      break;
    case AST_STATEMENT_FOR:
      mi_ast_statement_destroy(statement->as.for_stmt.init);
      mi_ast_expression_destroy(statement->as.for_stmt.condition);
      mi_ast_statement_destroy(statement->as.for_stmt.update);
      break;
    case AST_STATEMENT_WHILE:
      mi_ast_expression_destroy(statement->as.while_stmt.condition);
      mi_ast_statement_destroy(statement->as.while_stmt.body);
      break;
    case AST_STATEMENT_RETURN:
      mi_ast_expression_destroy(statement->as.expression);
      break;
    case AST_STATEMENT_FUNCTION_DECL:
      mi_ast_statement_list_destroy(statement->as.function_decl.body);
      mi_ast_statement_list_destroy(statement->as.function_decl.params);
      break;
    case AST_STATEMENT_FUNCTION_CALL:
      mi_ast_expression_list_destroy(statement->as.expression);
      break;
    case AST_STATEMENT_BLOCK:
      mi_ast_statement_list_destroy(statement->as.block_stmt);
      break;
    case AST_STATEMENT_PRINT:
      mi_ast_expression_destroy(statement->as.expression);
      break;
    case AST_STATEMENT_BREAK: break;
  }

  free(statement);
}


void mi_ast_statement_list_destroy(ASTStatement* list)
{
  ASTStatement* next = list;
  while (next != NULL)
  {
    ASTStatement* statement = next;
    next = statement->next;
    mi_ast_statement_destroy(statement); 
  }
}


void mi_ast_program_destroy(ASTProgram* program)
{
  if (program == NULL)
    return;

  mi_ast_statement_list_destroy(program->body);
  free(program);
}


void mi_ast_expression_list_destroy(ASTExpression* list)
{
  ASTExpression* next = list;
  while (next != NULL)
  {
    ASTExpression* expression = next;
    next = expression->next;
    mi_ast_expression_destroy(expression); 
  }
}


//
// Helper functions for ASTExpression nodes
//

ASTExpression* mi_ast_expression_create_term(ASTExpression* left, ASTTermOperator op, ASTExpression* right)
{
  ASSERT(left != NULL && (left->type == EXPR_TERM ||left->type == EXPR_FACTOR || EXPRESSION_IS_LITERAL_OR_LVALUE(left)));
  ASSERT(right == NULL || (right->type == EXPR_TERM || right->type == EXPR_FACTOR || EXPRESSION_IS_LITERAL_OR_LVALUE(right)));

  ASTExpression* expr = MI_ASTCREATE_NODE(ASTExpression);
  expr->type = EXPR_TERM;
  expr->as.term_expr.left   = left;
  expr->as.term_expr.right  = right;
  expr->as.term_expr.op     = op;
  expr->next = NULL;
  return expr;
}

ASTExpression* mi_ast_expression_create_factor(ASTExpression* left, ASTFactorOperator op, ASTExpression* right)
{
  ASSERT(left != NULL && (left->type == EXPR_TERM || EXPRESSION_IS_LITERAL_OR_LVALUE(left)));
  ASSERT(right != NULL && (right->type == EXPR_TERM || EXPRESSION_IS_LITERAL_OR_LVALUE(right)));

  ASTExpression* expr = MI_ASTCREATE_NODE(ASTExpression);
  expr->type = EXPR_FACTOR;
  expr->as.factor_expr.left   = left;
  expr->as.factor_expr.right  = right;
  expr->as.factor_expr.op     = op;
  expr->next = NULL;
  return expr;
}

ASTExpression* mi_ast_expression_create_unary(ASTUnaryOperator op, ASTExpression* expression) 
{
  ASSERT(expression != NULL); 

  ASTExpression* expr = MI_ASTCREATE_NODE(ASTExpression);
  expr->type = EXPR_UNARY;
  expr->as.unary_expr.op = op;
  expr->as.unary_expr.expression = expression;
  expr->next = NULL;
  return expr;
}

ASTExpression* mi_ast_expression_create_logical(ASTExpression* left, ASTLogicalOperator op, ASTExpression* right) 
{
  ASSERT(left != NULL && (left->type == EXPR_TERM || EXPRESSION_IS_LITERAL_OR_LVALUE(left)));
  ASSERT(right != NULL && (right->type == EXPR_TERM || EXPRESSION_IS_LITERAL_OR_LVALUE(right)));
  ASTExpression* expr = MI_ASTCREATE_NODE(ASTExpression);
  expr->type = EXPR_LOGICAL;
  expr->as.logical_expr.left = left;
  expr->as.logical_expr.op = op;
  expr->as.logical_expr.right = right;
  expr->next = NULL;
  return expr;
}

ASTExpression* mi_ast_expression_create_comparison(ASTExpression* left, ASTComparisonOperator op, ASTExpression* right) 
{
  ASTExpression* expr = MI_ASTCREATE_NODE(ASTExpression);
  expr->type = EXPR_COMPARISON;
  expr->as.comparison_expr.left = left;
  expr->as.comparison_expr.op = op;
  expr->as.comparison_expr.right = right;
  expr->next = NULL;
  return expr;
}

ASTExpression* mi_ast_expression_create_literal_bool(bool value)
{
  ASTExpression* expr = MI_ASTCREATE_NODE(ASTExpression);
  expr->type = EXPR_LITERAL_BOOL;
  expr->as.number_literal = (int) value;
  expr->next = NULL;
  return expr;
}

ASTExpression* mi_ast_expression_create_literal_int(int value) 
{
  ASTExpression* expr = MI_ASTCREATE_NODE(ASTExpression);
  expr->type = EXPR_LITERAL_INT;
  expr->as.number_literal = (double) value;
  expr->next = NULL;
  return expr;
}

ASTExpression* mi_ast_expression_create_literal_float(double value) 
{
  ASTExpression* expr = MI_ASTCREATE_NODE(ASTExpression);
  expr->type = EXPR_LITERAL_FLOAT;
  expr->as.number_literal = value;
  expr->next = NULL;
  return expr;
}

ASTExpression* mi_ast_expression_create_literal_string(const char* value) 
{
  ASTExpression* expr = MI_ASTCREATE_NODE(ASTExpression);
  expr->type = EXPR_LITERAL_STRING;
  expr->as.string_literal = strdup(value);
  expr->next = NULL;
  return expr;
}

ASTExpression* mi_ast_expression_create_lvalue(const char* identifier) 
{
  ASTExpression* expr = MI_ASTCREATE_NODE(ASTExpression);
  expr->type = EXPR_LVALUE;
  smallstr(&expr->as.identifier, identifier);
  expr->next = NULL;
  return expr;
}


ASTExpression* mi_ast_expression_create_function_call(const char* identifier, ASTExpression* args)
{
  ASTExpression* expr = MI_ASTCREATE_NODE(ASTExpression);
  expr->type = EXPR_FUNCTION_CALL;
  smallstr(&expr->as.func_call_expr.identifier, identifier);
  expr->as.func_call_expr.args = args;
  expr->next = NULL;
  return expr;
}



//
// Helper functions for ASTStatement nodes
//

ASTStatement* mi_ast_statement_create_assignment(const char* identifier, ASTExpression* expression) 
{
  ASTStatement* stmt = MI_ASTCREATE_NODE(ASTStatement);
  stmt->type = AST_STATEMENT_ASSIGNMENT;
  smallstr(&stmt->as.assignment.identifier, identifier);
  stmt->as.assignment.expression = expression;
  stmt->next = NULL;
  return stmt;
}

ASTStatement* mi_ast_statement_create_if(ASTExpression* condition, ASTStatement* if_branch, ASTStatement* else_branch) 
{
  ASSERT(if_branch != NULL);
  if (else_branch)
    ASSERT(else_branch->type != AST_STATEMENT_BLOCK);

  ASTStatement* stmt = MI_ASTCREATE_NODE(ASTStatement);
  stmt->type = AST_STATEMENT_IF;
  stmt->as.if_stmt.condition = condition;
  stmt->as.if_stmt.if_branch = if_branch;
  stmt->as.if_stmt.else_branch = else_branch;
  stmt->next = NULL;
  return stmt;
}

ASTStatement* mi_ast_statement_create_for(ASTStatement* init, ASTExpression* condition, ASTStatement* update, ASTStatement* body) 
{
  ASTStatement* stmt = MI_ASTCREATE_NODE(ASTStatement);
  stmt->type = AST_STATEMENT_FOR;
  stmt->as.for_stmt.init = init;
  stmt->as.for_stmt.condition = condition;
  stmt->as.for_stmt.update = update;
  stmt->as.for_stmt.body = body;
  stmt->next = NULL;
  return stmt;
}

ASTStatement* mi_ast_statement_create_while(ASTExpression* condition, ASTStatement* body) 
{
  ASTStatement* stmt = MI_ASTCREATE_NODE(ASTStatement);
  stmt->type = AST_STATEMENT_WHILE;
  stmt->as.while_stmt.condition = condition;
  stmt->as.while_stmt.body = body;
  stmt->next = NULL;
  return stmt;
}

ASTStatement* mi_ast_statement_create_return(ASTExpression* expression) 
{
  ASTStatement* stmt = MI_ASTCREATE_NODE(ASTStatement);
  stmt->type = AST_STATEMENT_RETURN;
  stmt->as.expression = expression;
  stmt->next = NULL;
  return stmt;
}

ASTStatement* mi_ast_statement_create_function_decl(const char* identifier, ASTStatement* params, ASTStatement* body) 
{
  ASTStatement* stmt = MI_ASTCREATE_NODE(ASTStatement);
  stmt->type = AST_STATEMENT_FUNCTION_DECL;
  stmt->as.function_decl.identifier = strdup(identifier);
  stmt->as.function_decl.params = params;
  stmt->as.function_decl.body = body;
  stmt->next = NULL;
  return stmt;
}

ASTStatement* mi_ast_statement_create_break(void)
{
  ASTStatement* stmt = MI_ASTCREATE_NODE(ASTStatement);
  stmt->type = AST_STATEMENT_BREAK;
  stmt->next = NULL;
  return stmt;
}

ASTStatement* mi_ast_statement_create_function_call(ASTExpression* func_call_expr)
{
  ASSERT(func_call_expr != NULL);
  ASSERT(func_call_expr->type == EXPR_FUNCTION_CALL);

  ASTStatement* stmt = MI_ASTCREATE_NODE(ASTStatement);
  stmt->type = AST_STATEMENT_FUNCTION_CALL;
  stmt->as.expression = func_call_expr;
  stmt->next = NULL;
  return stmt;
}

ASTProgram* mi_ast_program_create(ASTStatement* body) 
{
  ASTProgram* program = MI_ASTCREATE_NODE(ASTProgram);
  program->body = body;
  return program;
}

