/**
 * @file eval.h
 * @brief Provides structures and functions for interpreting and executing AST nodes.
 *
 * Defines the symbol table for variable management, error handling, and evaluation 
 * functions to execute expressions and statements in the AST.
 *
 * This header organizes functions into groups based on their prefixes:
 *
 * - mi_symbol_table_*: Functions for managing the symbol table, including 
 *   initialization, variable/function storage, and retrieval by identifier.
 *
 * - mi_runtime_*: Functions for creating runtime values for program evaluation and
 *   initializing `MiValue` types as needed.
 *
 * @author marciovmf
 */

#ifndef MINIMA_EVAL_H
#define MINIMA_EVAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "minima_ast.h"
#include "common.h"

//
// Symbol Table
//

#ifndef MI_Mi_MAX_SYMBOLS
#define MI_Mi_MAX_SYMBOLS       100
#endif


/**
 * @enum RuntimeError
 * @brief Defines error codes for runtime evaluation errors.
 */
typedef enum
{
  MI_ERROR_SUCCESS                            = 0,       // No error.
  MI_ERROR_NOT_IMPLEMENTED                    = 1,   // Error for an unimplemented feature.
  MI_ERROR_DIVIDE_BY_ZERO                     = 2,   // Error for division by zero.
  MI_ERROR_UNSUPPORTED_OPERATION              = 3,   // Error for an unsupported operation.
  MI_ERROR_UNINITIALIZED_VARIABLE_ACCESS      = 4,   // Error for acessing an uninitialized variable

  MI_ERROR_COUNT_
} MiError;

typedef enum MiValueType_e
{
  MI_VAL_INT,
  MI_VAL_FLOAT,
  MI_VAL_STRING,
  MI_VAL_BOOL,
  MI_VAL_VOID,
  MI_VAL_ANY // used of function parameters
} MiValueType;

typedef enum MiSymbolType_e
{
  MI_SYMBOL_VARIABLE,
  MI_SYMBOL_FUNCTION
} MiSymbolType;

typedef struct MiValue_t
{
  MiError error_code;
  MiValueType type;
  union
  {
    char* string_value;
    double number_value; // used for float, double and bool
  } as;
} MiValue;

typedef struct MiVariable_t
{
  char*     name;
  MiValue   value;
  int       scopeLevel;
} MiVariable;

typedef struct MiFunction_t
{
  char*       name;
  int         param_count;
  MiVariable* parameters;
  MiValue (*function_ptr)(int param_count, MiValue* parameters); 
} MiFunction;

typedef struct MiSymbol_t
{
  Smallstr identifier;
  MiSymbolType type;
  union
  {
    MiVariable variable;
    MiFunction function;
  } as;
} MiSymbol;

typedef struct SymbolTable_t
{
  MiSymbol entry[MI_Mi_MAX_SYMBOLS];    // Array of symbols (variables).
  int count;                            // Number of variables currently stored.
} MiSymbolTable;


//
// Public functions
//

/**
 * @brief Initializes the symbol table.
 * 
 * @param table Pointer to the symbol table to initialize.
 */
void mi_symbol_table_init(MiSymbolTable* table);

/**
 * @brief Retrieves a variable from the symbol table by its identifier.
 * 
 * @param table Pointer to the symbol table.
 * @param identifier Name of the variable to retrieve.
 * @return Pointer to the Symbol if found, or NULL if not found.
 */
MiSymbol* mi_symbol_table_get_variable(MiSymbolTable* table, const char* identifier);

/**
 * @brief Evaluates the entire program by iterating through the AST.
 * 
 * @param table Pointer to the symbol table for variable lookup.
 * @param program Pointer to the AST program node to evaluate.
 * @return Exit code or runtime status of the program execution.
 */
int mi_eval_program(MiSymbolTable* table, ASTProgram* program);

/**
 * @brief Creates a boolean runtime value
 * 
 * @param value The value to use when creating the runtime value
 * @return The runtime value
 */
MiValue mi_runtime_value_create_bool(bool value);

/**
 * @brief Creates a int runtime value
 * 
 * @param value The value to use when creating the runtime value
 * @return The runtime value
 */
MiValue mi_runtime_value_create_int(int value);

/**
 * @brief Creates a float runtime value
 * 
 * @param value The value to use when creating the runtime value
 * @return The runtime value
 */
MiValue mi_runtime_value_create_float(double value);

/**
 * @brief Creates a string runtime value
 * 
 * @param value The value to use when creating the runtime value
 * @return The runtime value
 */
MiValue mi_runtime_value_create_string(char* value);

/**
 * @brief Creates a void runtime value
 * 
 * @return The runtime value
 */
MiValue mi_runtime_value_create_void(void);

#ifdef __cplusplus
extern {
#endif

#endif  // MINIMA_EVAL_H

