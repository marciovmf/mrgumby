/**
 * @file ast.h
 * @brief Defines core structures and functions for Abstract Syntax Tree (AST) nodes.
 *
 * Provides data structures and utilities for expressions, statements, 
 * and program constructs in a language's AST.
 *
 * @author marciovmf
 */

#ifndef AST_H
#define AST_H

#include "common.h"   // for Smallstr

typedef struct ASTExpression_t            ASTExpression;
typedef struct ASTStatement_t             ASTStatement;
typedef struct ASTFunctionDecl_t          ASTFunctionDecl;
typedef struct ASTAssignment_t            ASTAssignment;
typedef struct ASTIfStatement_t           ASTIfStatement;
typedef struct ASTForStatement_t          ASTForStatement;
typedef struct ASTWhileStatement_t        ASTWhileStatement;
typedef struct ASTReturnStatement_t       ASTReturnStatement;
typedef struct ASTUnaryExpression_t       ASTUnaryExpression;
typedef struct ASTFunctionCallExpression_t ASTFunctionCallExpression;
typedef struct ASTComparisonExpression_t  ASTComparisonExpression;
typedef struct ASTLogicalExpression_t     ASTLogicalExpression;
typedef struct ASTFactor_t                ASTFactor; 
typedef struct ASTTerm_t                  ASTTerm;
typedef struct ASTProgram_t               ASTProgram;

typedef enum ASTFactorOperator_e
{
  OP_ADD          = 0,  // Addition
  OP_SUBTRACT     = 1,  // Subtraction
} ASTFactorOperator;

typedef enum ASTTermOperator_e
{
  OP_MULTIPLY     = 2,  // Multiplication
  OP_DIVIDE       = 3,  // Division
  OP_MOD          = 4,  // Modulus
  //OP_ASSIGN      // Assignment
} ASTTermOperator;

typedef enum ASTUnaryOperator_e
{
  OP_UNARY_PLUS   = 5,  // Unary plus
  OP_UNARY_MINUS  = 6,  // Unary minus
  OP_LOGICAL_NOT  = 7,  // Logical NOT
} ASTUnaryOperator;

typedef enum ASTComparisonOperator_e
{
  OP_LT           = 9,  // Less than
  OP_GT           = 10, // Greater than
  OP_LTE          = 11, // Less than or equal to
  OP_GTE          = 12, // Greater than or equal to
  OP_EQ           = 13, // Equal to
  OP_NEQ          = 14, // Not equal to
} ASTComparisonOperator;

typedef enum ASTLogicalOperator_e
{
  OP_LOGICAL_AND,  // Logical AND
  OP_LOGICAL_OR    // Logical OR
} ASTLogicalOperator;

typedef enum ASTStatementType_e
{
  AST_STATEMENT_ASSIGNMENT,      // Assignment
  AST_STATEMENT_IF,              // If statement
  AST_STATEMENT_FOR,             // For loop
  AST_STATEMENT_WHILE,           // While loop
  AST_STATEMENT_RETURN,          // Return statement
  AST_STATEMENT_FUNCTION_DECL,   // Function declaration
  AST_STATEMENT_BLOCK,           // Block of statements
  AST_STATEMENT_PRINT,           // Print statement
  AST_STATEMENT_INPUT,           // Input statement
  AST_STATEMENT_FUNCTION_CALL,   // Function call
  AST_STATEMENT_BREAK            // Break statement
} ASTStatementType;

typedef enum ASTExpressionType_e
{
  EXPR_VOID           = 0,      // Void expression
  EXPR_UNARY          = 1,      // Unary expression
  EXPR_COMPARISON     = 2,      // Comparison expression
  EXPR_LOGICAL        = 3,      // Logical expression
  EXPR_FACTOR         = 4,      // Factor for high precedence
  EXPR_TERM           = 5,      // Term for low precedence
  EXPR_LITERAL_BOOL   = 6,      // Bool literal
  EXPR_LITERAL_INT    = 7,      // Integer literal
  EXPR_LITERAL_FLOAT  = 8,      // Floating-point literal
  EXPR_LITERAL_STRING = 9,      // String literal
  EXPR_LVALUE         = 10,     // Variable reference
  EXPR_FUNCTION_CALL  = 11,     // Function call
} ASTExpressionType;

struct ASTUnaryExpression_t
{
  ASTExpression* expression;
  ASTUnaryOperator op;
};

struct ASTComparisonExpression_t
{
  ASTExpression* left;
  ASTExpression* right;
  ASTComparisonOperator op;
};

struct ASTFactor_t
{
  ASTExpression* left;
  ASTExpression* right;
  ASTFactorOperator op;
};

struct ASTTerm_t
{
  ASTExpression* left;
  ASTExpression* right;
  ASTTermOperator op;
};

struct ASTLogicalExpression_t
{
  ASTExpression* left;
  ASTExpression* right;
  ASTLogicalOperator op;
};

struct ASTFunctionCallExpression_t
{
  Smallstr identifier;
  ASTExpression* args;
};

struct ASTExpression_t
{
  ASTExpressionType type;                   // Expression type
  union 
  {
    ASTUnaryExpression        unary_expr;     // Unary expression
    ASTComparisonExpression   comparison_expr;// Comparison expression
    ASTLogicalExpression      logical_expr;   // Logical expression
    ASTFunctionCallExpression func_call_expr; // Function call expression
    ASTFactor                 factor_expr;    // Factor expression
    ASTTerm                   term_expr;      // Term expression
    double                    number_literal; // Numeric literal
    char*                     string_literal; // String literal
    Smallstr                  identifier;     // Variable identifier
  } as;
  ASTExpression* next;
};

struct ASTAssignment_t
{
  Smallstr identifier;
  ASTExpression* expression;
};

struct ASTIfStatement_t
{
  ASTExpression* condition;
  ASTStatement* if_branch;
  ASTStatement* else_branch;
};

struct ASTForStatement_t
{
  ASTStatement* init;         // Assignment
  ASTExpression* condition;   // Comparison expression
  ASTStatement* update;       // ASsigment
  ASTStatement* body;         // Statement list
};

struct ASTWhileStatement_t
{
  ASTExpression* condition;
  ASTStatement* body;
};

struct ASTReturnStatement_t
{
  ASTExpression* condition;
};

struct ASTFunctionDecl_t
{
  char*             identifier;
  ASTStatement*     params;
  ASTStatement*     body;
};

struct ASTStatementList_t 
{
  ASTStatement**  statements; // Array of pointers to ASTStatements
  size_t          count;      // Number of statements in the array
  size_t          capacity;   // Capacity of the array
};

struct ASTStatement_t
{
  ASTStatementType type; // Statement type
  union 
  {
    ASTAssignment       assignment;     // Assignment
    ASTIfStatement      if_stmt;        // If statement
    ASTForStatement     for_stmt;       // For loop
    ASTWhileStatement   while_stmt;     // While statement
    ASTFunctionDecl     function_decl;  // Function declaration
    ASTStatement*       block_stmt;     // statement list
    ASTExpression*      expression;     // Return expression , Function call
  } as;

  ASTStatement* next;   // Next statement when this node is part of a statement list
};

struct ASTProgram_t
{
  ASTStatement* body;
};

//
// Helper functions for ASTExpression nodes
//
ASTProgram* ast_create_program(ASTStatement* statements);

ASTExpression* ast_create_expression_term(ASTExpression* left, ASTTermOperator op, ASTExpression* right);
ASTExpression* ast_create_expression_factor(ASTExpression* left, ASTFactorOperator op, ASTExpression* right);
ASTExpression* ast_create_expression_unary(ASTUnaryOperator op, ASTExpression* expression);
ASTExpression* ast_create_expression_logical(ASTExpression* left, ASTLogicalOperator op, ASTExpression* right);
ASTExpression* ast_create_expression_comparison(ASTExpression* left, ASTComparisonOperator op, ASTExpression* right);
ASTExpression* ast_create_expression_literal_bool(bool value);
ASTExpression* ast_create_expression_literal_int(int value);
ASTExpression* ast_create_expression_literal_float(double value);
ASTExpression* ast_create_expression_literal_string(const char* value);
ASTExpression* ast_create_expression_lvalue(const char* identifier);
ASTExpression* ast_create_expression_function_call(const char* identifier, ASTExpression* args);

//
// Helper functions for creating ASTStatement
//
ASTStatement* ast_create_statement_assignment(const char* identifier, ASTExpression* expression);
ASTStatement* ast_create_statement_if(ASTExpression* condition, ASTStatement* if_branch, ASTStatement* else_branch);
ASTStatement* ast_create_statement_for(ASTStatement* init, ASTExpression* condition, ASTStatement* update, ASTStatement* body);
ASTStatement* ast_create_statement_while(ASTExpression* condition, ASTStatement* body);
ASTStatement* ast_create_statement_return(ASTExpression* expression);
ASTStatement* ast_create_statement_function_decl(const char* identifier, ASTStatement* params, ASTStatement* body);
ASTStatement* ast_create_statement_function_call(ASTExpression* function_call_expression);
ASTStatement* ast_create_statement_print(ASTExpression* expression);
ASTStatement* ast_create_statement_input(const char* identifier);
ASTStatement* ast_create_statement_break(void);

//
// Helper functions for destroying nodes
//
void ast_destroy_expression(ASTExpression* expression);
void ast_destroy_statement(ASTStatement* statement);
void ast_destroy_program(ASTProgram* program);
void ast_destroy_statement_list(ASTStatement* stmt_list);
void ast_destroy_expression_list(ASTExpression* list);

#endif // AST_H
