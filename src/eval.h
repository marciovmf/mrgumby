/**
 * @file eval.h
 * @brief Provides structures and functions for interpreting and executing AST nodes.
 *
 * Defines the symbol table for variable management, error handling, and evaluation 
 * functions to execute expressions and statements in the AST.
 * 
 * @author marciovmf
 */

#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast.h"
#include "common.h"

//
// Symbol Table
//

#ifndef RT_MAX_SYMBOLS
#define RT_MAX_SYMBOLS       100
#endif


/**
 * @enum RuntimeError
 * @brief Defines error codes for runtime evaluation errors.
 */
typedef enum
{
  RT_ERROR_SUCCESS               = 0,       // No error.
  RT_ERROR_DIVIDE_BY_ZERO        = -1024,   // Error for division by zero.
  RT_ERROR_UNSUPPORTED_OPERATION = -1025,   // Error for an unsupported operation.
  RT_ERROR_NOT_IMPLEMENTED       = -1026,   // Error for an unimplemented feature.
} RTError;

typedef enum RTValueType_t
{
  RT_VAL_INT,
  RT_VAL_FLOAT,
  RT_VAL_STRING,
  RT_VAL_BOOL,
  RT_VAL_VOID,
  RT_VAL_ANY // used of function parameters
} RTValueType;

typedef struct RTValue_t
{
  RTError error_code;
  RTValueType type;
  union
  {
    char* string_value;
    double number_value; // used for float, double and bool
  } as;
} RTValue;

typedef struct RTVariable_t
{
  char*     name;
  RTValue   value;
  int       scopeLevel;
} RTVariable;

typedef struct RTFunction_t
{
  char*       name;
  int         param_count;
  RTVariable* parameters;
  RTValue (*function_ptr)(int param_count, RTValue* parameters); 
} RTFunction;

typedef struct RTSymbol_t
{
  Smallstr identifier;
  enum { RT_SYMBOL_VARIABLE, RT_SYMBOL_FUNCTION } type;
  union
  {
    RTVariable variable;
    RTFunction function;
  } as;
} RTSymbol;

typedef struct SymbolTable_t
{
  RTSymbol entry[RT_MAX_SYMBOLS];   // Array of symbols (variables).
  int count;                        // Number of variables currently stored.
} RTSymbolTable;


/**
 * @brief Initializes the symbol table.
 * 
 * @param table Pointer to the symbol table to initialize.
 */
void symbol_table_init(RTSymbolTable* table);

/**
 * @brief Retrieves a variable from the symbol table by its identifier.
 * 
 * @param table Pointer to the symbol table.
 * @param identifier Name of the variable to retrieve.
 * @return Pointer to the Symbol if found, or NULL if not found.
 */
RTSymbol* symbol_table_get_variable(RTSymbolTable* table, const char* identifier);


//
// Public functions
//


/**
 * @brief Evaluates the entire program by iterating through the AST.
 * 
 * @param table Pointer to the symbol table for variable lookup.
 * @param program Pointer to the AST program node to evaluate.
 * @return Exit code or runtime status of the program execution.
 */
int eval_program(RTSymbolTable* table, ASTProgram* program);

/**
 * @brief Creates a boolean runtime value
 * 
 * @param value The value to use when creating the runtime value
 * @return The runtime value
 */
RTValue runtime_value_create_bool(bool value);

/**
 * @brief Creates a int runtime value
 * 
 * @param value The value to use when creating the runtime value
 * @return The runtime value
 */
RTValue runtime_value_create_int(int value);

/**
 * @brief Creates a float runtime value
 * 
 * @param value The value to use when creating the runtime value
 * @return The runtime value
 */
RTValue runtime_value_create_float(double value);

/**
 * @brief Creates a string runtime value
 * 
 * @param value The value to use when creating the runtime value
 * @return The runtime value
 */
RTValue runtime_value_create_string(char* value);

/**
 * @brief Creates a void runtime value
 * 
 * @return The runtime value
 */
RTValue runtime_value_create_void(void);


#endif  // INTERPRETER_H

