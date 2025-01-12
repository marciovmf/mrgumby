/**
 * @file minima_eval.h
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

#include "common.h"
#include "minima_common.h"
#include "minima_array.h"
#include "minima_ast.h"

#include <stdio.h>

//
// Symbol Table
//


/**
 * @enum RuntimeError
 * @brief Defines error codes for runtime evaluation errors.
 */
typedef enum
{
  MI_ERROR_SUCCESS                            = 0,   // No error.
  MI_ERROR_NOT_IMPLEMENTED                    = 1,   // Error for an unimplemented feature.
  MI_ERROR_DIVIDE_BY_ZERO                     = 2,   // Error for division by zero.
  MI_ERROR_UNSUPPORTED_OPERATION              = 3,   // Error for an unsupported operation.
  MI_ERROR_UNINITIALIZED_VARIABLE_ACCESS      = 4,   // Error for acessing an uninitialized variable
  MI_ERROR_ARRAY_INDEX_TYPE                   = 5,   // Error for acessing an array with a non integer index type
  MI_ERROR_ARRAY_INDEX_OUT_OF_BOUNDS          = 6,   // Error for indexing out array bounds
  MI_ERROR_INCORRECT_ARGUMENT_COUNT           = 7,   // Error for passing incorrect number of arguments to a function
  MI_ERROR_INCORRECT_ARGUMENT_TYPE            = 8,   // Error for passing wrong argument type to a function
MI_ERROR_INDEXING_NON_ARRAY_TYPE              = 9,   // Error for indexing a non array type
  MI_ERROR_COUNT_
} MiError;

typedef enum MiSymbolType_e
{
  MI_SYMBOL_VARIABLE  = 1,
  MI_SYMBOL_FUNCTION  = 3,
} MiSymbolType;

typedef struct MiValue_t
{
  MiError error_code;
  MiType type;
  union
  {
    char* string_value;
    double number_value; // used for float, double and bool
    MiArray* array_value;
  } as;
} MiValue;

typedef struct MiVariable_t
{
  const char*     name;
  MiValue   value;
  unsigned int scope;
} MiVariable;

typedef MiValue (*MiFunctionPtr)(int param_count, MiValue* parameters, FILE* out);

typedef struct MiFunction_t
{
  bool          variadic;
  char*         name;
  unsigned int  param_count;
  MiVariable*   parameters;
  MiFunctionPtr function_ptr;
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
  MiSymbol* entry;    // Array of symbols (variables).
  unsigned int scope; // current scope level
  size_t count;       // Number of variables currently stored.
  size_t capacity;
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
 * @brief Creates a new function symbol in the symbol table.
 *
 * @param table Pointer to the symbol table where the function will be added.
 * @param identifier Name of the function to be created.
 * @param arg_count Number of parameters the function will accept.
 * @return Pointer to the newly created Symbol.
 */
MiSymbol* mi_symbol_table_create_function(MiSymbolTable* table, MiFunctionPtr function_ptr, const char* identifier, int arg_count, bool variadic);

/**
 * @brief Sets the details of a parameter for a function symbol.
 *
 * @param symbol Pointer to the function Symbol to modify.
 * @param index Index of the parameter to set (0-based).
 * @param param_name Name of the parameter.
 * @param param_type Type of the parameter (e.g., `MI_TYPE_ANY`).
 * @param param_scope Scope of the parameter (e.g., local or global).
 */
void mi_symbol_table_function_set_param(MiSymbol* symbol, unsigned int index, const char* param_name, MiType param_type);


void mi_symbol_table_destroy(MiSymbolTable* table);

/**
 * @brief Evaluates the entire program by iterating through the AST.
 * 
 * @param table Pointer to the symbol table for variable lookup.
 * @param program Pointer to the AST program node to evaluate.
 * @return Exit code or runtime status of the program execution.
 */
int mi_eval_program(MiSymbolTable* table, ASTProgram* program, FILE* out);

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

