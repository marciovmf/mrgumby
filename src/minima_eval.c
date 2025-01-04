#include "common.h"
#include "minima_array.h"
#include "minima_ast.h"
#include "minima_common.h"
#include "minima_eval.h"
#include <string.h>
#include <stdlib.h>


//
// Runtime expression value
//

MiValue mi_eval_statement_list(MiSymbolTable* table, ASTStatement* first_stmt);

const char* mi_error_name(MiError error)
{
  static const char *error_name[] =
  {
    [MI_ERROR_SUCCESS]                            = "Operation completed successfully",
    [MI_ERROR_NOT_IMPLEMENTED]                    = "Feature not implemented",
    [MI_ERROR_DIVIDE_BY_ZERO]                     = "Division by zero",
    [MI_ERROR_UNSUPPORTED_OPERATION]              = "Operation is not supported",
    [MI_ERROR_UNINITIALIZED_VARIABLE_ACCESS]      = "Attempted access to an uninitialized variable",
    [MI_ERROR_ARRAY_INDEX_TYPE]                   = "Array index must be integer",
    [MI_ERROR_ARRAY_INDEX_OUT_OF_BOUNDS]          = "Array index out of bounds",
    [MI_ERROR_INCORRECT_ARGUMENT_COUNT]           = "Incorrect number of arguments for function",
    [MI_ERROR_INCORRECT_ARGUMENT_TYPE]            = "Incorrect argument type for function",
    [MI_ERROR_INDEXING_NON_ARRAY_TYPE]            = "Indexing non array type",
  };

  if (error >= 0 && error < MI_ERROR_COUNT_)
  {
    return error_name[error];
  }
  return "Unknown error";
}

MiValue mi_ast_expression_create_break()
{
  return (MiValue){ .type = MI_TYPE_INTERNAL_BREAK, .error_code = MI_ERROR_SUCCESS };
}

MiValue mi_runtime_value_create_array(MiArray* value)
{
  return (MiValue){ .type = MI_TYPE_ARRAY, .as.array_value = value, .error_code = MI_ERROR_SUCCESS };
}

MiValue mi_runtime_value_create_bool(bool value)
{
  return (MiValue){ .type = MI_TYPE_BOOL, .as.number_value = value, .error_code = MI_ERROR_SUCCESS };
}

MiValue mi_runtime_value_create_int(int value)
{
  return (MiValue){ .type = MI_TYPE_INT, .as.number_value = value, .error_code = MI_ERROR_SUCCESS };
}

MiValue mi_runtime_value_create_float(double value)
{
  return (MiValue){ .type = MI_TYPE_FLOAT, .as.number_value = value, .error_code = MI_ERROR_SUCCESS};
}

MiValue mi_runtime_value_create_string(char* value)
{
  return (MiValue){ .type = MI_TYPE_STRING, .as.string_value = value, .error_code = MI_ERROR_SUCCESS};
}

MiValue mi_runtime_value_create_void(void)
{
  return (MiValue){ .type = MI_TYPE_VOID, .error_code = MI_ERROR_SUCCESS};
}

inline MiValue mi_runtime_value_create_error(MiError error)
{
  return (MiValue){ .type = MI_TYPE_VOID, .error_code = error};
}

inline void s_symbol_table_grow(MiSymbolTable* table)
{
  if (table->count >= table->capacity)
  {
    table->capacity *= 2;
    table->entry = realloc(table->entry, table->capacity * sizeof(MiSymbol));
  }
}

void mi_symbol_table_init(MiSymbolTable* table) 
{
  table->scope = 0;
  table->count = 0;
  table->capacity = 64;
  table->entry = (MiSymbol*) malloc(table->capacity * sizeof(MiSymbol));
}

void mi_symbol_table_destroy(MiSymbolTable* table)
{
  for (size_t i = 0; i < table->count; ++i)
  {
    if (table->entry[i].type == MI_SYMBOL_FUNCTION)
    {
      unsigned int argc = table->entry[i].as.function.param_count;
      for (unsigned int j = 0; j < argc; ++j)
      {
        free((void*)table->entry[i].as.function.parameters[j].name);
      }
      free(table->entry[i].as.function.parameters);
    }
  }
  free(table->entry);
}

static void s_mi_symbol_table_scope_begin(MiSymbolTable* table)
{
  table->scope++;
}

static void s_mi_symbol_table_scope_end(MiSymbolTable* table)
{
  //TODO: Destroy out of scope variables;
  table->scope--;
}

inline MiSymbol* s_symbol_table_get_variable(MiSymbolTable* table, const char* identifier) 
{
  for (unsigned int i = 0; i < table->count; i++) 
  {
    if (strcmp(table->entry[i].identifier.str, identifier) == 0 && table->entry[i].type == MI_SYMBOL_VARIABLE && table->entry[i].as.variable.scope <= table->scope) 
    {
      return &table->entry[i];
    }
  }
  return NULL;
}

MiSymbol* mi_symbol_table_create_function(MiSymbolTable* table, MiFunctionPtr function_ptr, const char* identifier, int arg_count, bool variadic)
{
  s_symbol_table_grow(table);
  MiSymbol* symbol = &table->entry[table->count++];
  smallstr(&symbol->identifier, identifier);
  symbol->type = MI_SYMBOL_FUNCTION;
  symbol->as.function.param_count = arg_count;
  symbol->as.function.function_ptr = function_ptr;
  symbol->as.function.parameters = malloc(arg_count * sizeof(MiVariable));
  symbol->as.function.variadic = variadic;

  // Initialize parameters with default values
  for (int i = 0; i < arg_count; ++i)
  {
    symbol->as.function.parameters[i].name = NULL;
    symbol->as.function.parameters[i].value.type = MI_TYPE_ANY;
    symbol->as.function.parameters[i].scope = 0;
  }
  return symbol;
}

void mi_symbol_table_function_set_param(MiSymbol* symbol, unsigned int index, const char* param_name, MiType param_type)
{
  ASSERT(symbol->type == MI_SYMBOL_FUNCTION);
  ASSERT(index >= 0 && index < symbol->as.function.param_count);
  MiVariable* param = &symbol->as.function.parameters[index];
  param->name = param_name;
  param->value.type = param_type;
  param->scope = 1;
}

MiSymbol* mi_symbol_table_get_variable(MiSymbolTable* table, const char* identifier) 
{
  MiSymbol* variable = s_symbol_table_get_variable(table, identifier);
  if (variable == NULL)
    log_error("Requested uninitialized variable '%s'\n", identifier);
  return variable;
}

MiSymbol* mi_symbol_table_get_function(MiSymbolTable* table, const char* identifier) 
{
  for (unsigned int i = 0; i < table->count; i++) 
  {
    if (strcmp(table->entry[i].identifier.str, identifier) == 0 && table->entry[i].type == MI_SYMBOL_FUNCTION) 
    {
      return &table->entry[i];
    }
  }

  log_warning("Requested unknown function '%s'\n", identifier);
  return 0;
}

void mi_symbol_table_set_variable_bool(MiSymbolTable* table, const char* identifier, bool value)
{
  MiSymbol* variable = s_symbol_table_get_variable(table, identifier);
  if (variable != NULL)
  {
    variable->as.variable.value = mi_runtime_value_create_bool(value);
  }
  else
  {
    smallstr(&table->entry[table->count].identifier, identifier);
    table->entry[table->count].as.variable.scope = table->scope;
    table->entry[table->count].type = MI_SYMBOL_VARIABLE;
    table->entry[table->count].as.variable.name = table->entry[table->count].identifier.str;
    table->entry[table->count].as.variable.value = mi_runtime_value_create_bool(value);
    table->count++;
  }
}

void mi_symbol_table_set_variable_int(MiSymbolTable* table, const char* identifier, int value) 
{
  MiSymbol* variable = s_symbol_table_get_variable(table, identifier);
  if (variable != NULL)
  {
    variable->as.variable.value = mi_runtime_value_create_int(value);
  }
  else
  {
    smallstr(&table->entry[table->count].identifier, identifier);
    table->entry[table->count].as.variable.scope = table->scope;
    table->entry[table->count].type = MI_SYMBOL_VARIABLE;
    table->entry[table->count].as.variable.name = table->entry[table->count].identifier.str;
    table->entry[table->count].as.variable.value = mi_runtime_value_create_int(value);
    table->count++;
  }
}

void mi_symbol_table_set_variable_float(MiSymbolTable* table, const char* identifier, double value) 
{
  MiSymbol* variable = s_symbol_table_get_variable(table, identifier);
  if (variable != NULL)
  {
    variable->as.variable.value = mi_runtime_value_create_float(value);
  }
  else
  {
    smallstr(&table->entry[table->count].identifier, identifier);
    table->entry[table->count].as.variable.scope = table->scope;
    table->entry[table->count].type = MI_SYMBOL_VARIABLE;
    table->entry[table->count].as.variable.name = table->entry[table->count].identifier.str;
    table->entry[table->count].as.variable.value = mi_runtime_value_create_float(value);
    table->count++;
  }
}

void mi_symbol_table_set_variable_string(MiSymbolTable* table, const char* identifier, char* value) 
{
  MiSymbol* variable = s_symbol_table_get_variable(table, identifier);
  if (variable != NULL)
  {
    variable->as.variable.value = mi_runtime_value_create_string(value);
  }
  else
  {
    smallstr(&table->entry[table->count].identifier, identifier);
    table->entry[table->count].as.variable.scope = table->scope;
    table->entry[table->count].type = MI_SYMBOL_VARIABLE;
    table->entry[table->count].as.variable.name = table->entry[table->count].identifier.str;
    table->entry[table->count].as.variable.value = mi_runtime_value_create_string(value);
    table->count++;
  }
}

void mi_symbol_table_set_variable_array(MiSymbolTable* table, const char* identifier, MiArray* value) 
{
  MiSymbol* variable = s_symbol_table_get_variable(table, identifier);
  if (variable != NULL)
  {
    variable->as.variable.value = mi_runtime_value_create_array(value);
  }
  else
  {
    smallstr(&table->entry[table->count].identifier, identifier);
    table->entry[table->count].as.variable.scope = table->scope;
    table->entry[table->count].type = MI_SYMBOL_VARIABLE;
    table->entry[table->count].as.variable.name = table->entry[table->count].identifier.str;
    table->entry[table->count].as.variable.value = mi_runtime_value_create_array(value);
    table->count++;
  }
}


//
// Evaluation functions
//

MiValue mi_eval_expression(MiSymbolTable* table, ASTExpression* expr) 
{
  switch (expr->type) 
  {
    case EXPR_LITERAL_BOOL:
      return mi_runtime_value_create_bool(((int) expr->as.number_literal) != 0);
    case EXPR_LITERAL_INT:
      return  mi_runtime_value_create_int((int) expr->as.number_literal);
    case EXPR_LITERAL_FLOAT:
      return mi_runtime_value_create_float(expr->as.number_literal);
    case EXPR_LITERAL_STRING:
      return mi_runtime_value_create_string(expr->as.string_literal);
    case EXPR_FUNCTION_CALL:
      {
        MiSymbol* symbol = mi_symbol_table_get_function(table, expr->as.func_call_expr.identifier.str);
        MiValue args[64];
        unsigned int argc = 0;

        ASTExpression* arg = expr->as.func_call_expr.args;
        while(arg != NULL)
        {

          if (argc >= symbol->as.function.param_count && symbol->as.function.variadic == false)
          {
            return mi_runtime_value_create_error(MI_ERROR_INCORRECT_ARGUMENT_COUNT);
          }

          MiValue v = mi_eval_expression(table, arg);
          if (v.error_code != MI_ERROR_SUCCESS)
            return v;

          if (symbol->as.function.parameters->value.type != v.type && symbol->as.function.parameters->value.type != MI_TYPE_ANY)
          {
            return mi_runtime_value_create_error(MI_ERROR_INCORRECT_ARGUMENT_TYPE);
          }
          args[argc++] = v;
          arg = arg->next;
        }

        return symbol->as.function.function_ptr(argc, (MiValue*) args);
        break;
      }
    case EXPR_FACTOR:
      {
        MiValue left = mi_eval_expression(table, expr->as.term_expr.left);
        MiValue right = mi_eval_expression(table, expr->as.term_expr.right);
        MiType resultType;

        if (left.type == MI_TYPE_STRING || right.type == MI_TYPE_STRING)
          resultType = MI_TYPE_STRING;
        if (left.type == MI_TYPE_FLOAT || right.type == MI_TYPE_FLOAT)
          resultType = MI_TYPE_FLOAT;
        else
          resultType = MI_TYPE_INT;
        switch (expr->as.factor_expr.op) 
        {
          case OP_ADD:
            {
              if (resultType == MI_TYPE_INT)
                return mi_runtime_value_create_int((int)(left.as.number_value + right.as.number_value));
              else if (resultType == MI_TYPE_FLOAT)
                return mi_runtime_value_create_float(left.as.number_value + right.as.number_value);
              else if (resultType == MI_TYPE_STRING)
                //TODO: String concatenation and conversion
                return mi_runtime_value_create_error(MI_ERROR_NOT_IMPLEMENTED);
              else
                ASSERT_BREAK();
            }
          case OP_SUBTRACT:
            {
              if (resultType == MI_TYPE_INT)
                return mi_runtime_value_create_int((int)(left.as.number_value - right.as.number_value));
              else if (resultType == MI_TYPE_FLOAT)
                return mi_runtime_value_create_float(left.as.number_value - right.as.number_value);
              else if (resultType == MI_TYPE_STRING)
                //TODO: String concatenation and conversion
                return mi_runtime_value_create_error(MI_ERROR_NOT_IMPLEMENTED);
              else
                ASSERT_BREAK();
            }
          default:
            log_and_break("Unknown binary operator for FACTOR");
            return mi_runtime_value_create_void();
            break;
        }
      }
      break;
    case EXPR_TERM: 
      {
        MiValue left =  mi_eval_expression(table, expr->as.factor_expr.left);
        MiValue right = mi_eval_expression(table, expr->as.factor_expr.right);
        MiType resultType;

        if (left.type == MI_TYPE_STRING || right.type == MI_TYPE_STRING)
          resultType = MI_TYPE_STRING;
        if (left.type == MI_TYPE_FLOAT || right.type == MI_TYPE_FLOAT)
          resultType = MI_TYPE_FLOAT;
        else
          resultType = MI_TYPE_INT;
        switch (expr->as.term_expr.op) 
        {
          case OP_MULTIPLY:
            {
              if (resultType == MI_TYPE_INT)
                return mi_runtime_value_create_int((int)(left.as.number_value * right.as.number_value));
              else if (resultType == MI_TYPE_FLOAT)
                return mi_runtime_value_create_float(left.as.number_value * right.as.number_value);
              else if (resultType == MI_TYPE_STRING)
                //TODO: String concatenation and conversion 
                return mi_runtime_value_create_error(MI_ERROR_NOT_IMPLEMENTED);
              else
                ASSERT_BREAK();
            }
          case OP_DIVIDE:
            {
              if (resultType != EXPR_LITERAL_STRING && right.as.number_value == 0.0)
                return mi_runtime_value_create_error(MI_ERROR_DIVIDE_BY_ZERO);

              if (resultType == MI_TYPE_INT)
                return mi_runtime_value_create_int((int)(left.as.number_value / right.as.number_value));
              else if (resultType == MI_TYPE_FLOAT)
                return mi_runtime_value_create_float(left.as.number_value / right.as.number_value);
              else if (resultType == MI_TYPE_STRING)
                //TODO: String concatenation and conversion 
                return mi_runtime_value_create_error(MI_ERROR_NOT_IMPLEMENTED);
              else
                ASSERT_BREAK();
            }
          case OP_MOD:
            {
              if (resultType == MI_TYPE_STRING)
                return mi_runtime_value_create_error(MI_ERROR_UNSUPPORTED_OPERATION);
              else
                return mi_runtime_value_create_int(((int)left.as.number_value % (int) right.as.number_value));
            }
          default:
            log_and_break("Unknown binary operator for TERM");
            return mi_runtime_value_create_void();
            break;
        }

      } break;
    case EXPR_LVALUE:
      {
        MiSymbol* lvalue = mi_symbol_table_get_variable(table, expr->as.lvalue.identifier.str);
        if (lvalue == NULL)
          return mi_runtime_value_create_error(MI_ERROR_UNINITIALIZED_VARIABLE_ACCESS);

        if (expr->as.lvalue.type == LVALUE_ARRAY_ACCESS)
        {
          ASTExpression* index_expression = expr->as.lvalue.index_expression;
          MiArray* array = lvalue->as.variable.value.as.array_value;
          MiArrayElement* element = NULL;

          while(index_expression)
          {
            MiValue index = mi_eval_expression(table, index_expression);

            if (index.type != MI_TYPE_INT && index.type != MI_TYPE_BOOL)
            {
              return mi_runtime_value_create_error(MI_ERROR_ARRAY_INDEX_TYPE);
            }

            if (((int) index.as.number_value) >= (int) array->size)
            {
              return mi_runtime_value_create_error(MI_ERROR_ARRAY_INDEX_OUT_OF_BOUNDS);
            }

            int i = (int) index.as.number_value;
            element = &array->elements[i];

            index_expression = index_expression->next;

            if (element->type == MI_TYPE_ARRAY)
              array = element->data.arr;
            else if (index_expression != NULL)
            {
              return mi_runtime_value_create_error(MI_ERROR_INDEXING_NON_ARRAY_TYPE);
            }
          }

          if (element->type == MI_TYPE_INT)
            return mi_runtime_value_create_int(element->data.i);
          else if (element->type == MI_TYPE_FLOAT)
            return  mi_runtime_value_create_float(element->data.f);
          else if (element->type == MI_TYPE_BOOL)
            return mi_runtime_value_create_bool((bool)element->data.i);
          else if (element->type == MI_TYPE_STRING)
            return mi_runtime_value_create_string(element->data.s);
          else if (element->type == MI_TYPE_ARRAY)
            return mi_runtime_value_create_array(element->data.arr);
          else
            ASSERT_BREAK();
        }
        return lvalue->as.variable.value;
        break;
      }
    case EXPR_UNARY:
      {
        MiValue value = mi_eval_expression(table, expr->as.unary_expr.expression);
        ASSERT(expr->as.unary_expr.op == OP_UNARY_MINUS || expr->as.unary_expr.op == OP_UNARY_PLUS || expr->as.unary_expr.op == OP_LOGICAL_NOT);
        if (expr->as.unary_expr.op == OP_UNARY_MINUS)
          value.as.number_value = -value.as.number_value;
        else if (expr->as.unary_expr.op == OP_UNARY_PLUS)
          value.as.number_value = +value.as.number_value;
        else if (expr->as.unary_expr.op == OP_LOGICAL_NOT)
          value.as.number_value =  !((bool)value.as.number_value);
        else
          ASSERT_BREAK();

        return value;
        break;
      }
    case EXPR_LOGICAL:
      {
        //TODO: validate conversion between types for boolean comparisons
        MiValue left = mi_eval_expression(table, expr->as.term_expr.left);
        MiValue right = mi_eval_expression(table, expr->as.term_expr.right);

        switch (expr->as.logical_expr.op)
        {
          case OP_LOGICAL_OR:
            return mi_runtime_value_create_bool(left.as.number_value || right.as.number_value);
            break;
          case OP_LOGICAL_AND:
            return mi_runtime_value_create_bool(left.as.number_value && right.as.number_value);
            break;
          default:
            log_and_break("Unknown expression type");
            break;
        }

        break;
      }
    case EXPR_COMPARISON:
      {
        //TODO: validate conversion between types for boolean comparisons
        MiValue left = mi_eval_expression(table, expr->as.term_expr.left);
        MiValue right = mi_eval_expression(table, expr->as.term_expr.right);

        switch (expr->as.comparison_expr.op)
        {
          case OP_LT:
            return mi_runtime_value_create_bool(left.as.number_value < right.as.number_value);
            break;
          case OP_GT:
            return mi_runtime_value_create_bool(left.as.number_value > right.as.number_value);
            break;
          case OP_LTE:
            return mi_runtime_value_create_bool(left.as.number_value <= right.as.number_value);
            break;
          case OP_GTE:
            return mi_runtime_value_create_bool(left.as.number_value <= right.as.number_value);
            break;
          case OP_EQ:
            return mi_runtime_value_create_bool(left.as.number_value == right.as.number_value);
            break;
          case OP_NEQ:
            return mi_runtime_value_create_bool(left.as.number_value != right.as.number_value);
            break;
          default:
            log_and_break("Unknown expression type");
            break;
        }

        break;
      }
    default:
      log_and_break("Unknown expression type %d\n", expr->type);
  }
  return mi_runtime_value_create_void();
}

MiValue mi_eval_statement(MiSymbolTable* table, ASTStatement* stmt) 
{
  switch (stmt->type) 
  {
    case AST_STATEMENT_RAW: 
      {
        printf("%.*s", (int) stmt->as.raw.len, stmt->as.raw.start);
        return mi_runtime_value_create_void();
      }
    case AST_STATEMENT_ASSIGNMENT: 
      {
        MiValue value;

        // This is an array initialization
        if (stmt->as.assignment.rvalue->type == EXPR_ARRAY_INIT)
        {
          MiArray* array = mi_array_create(4);
          ASTExpression* arg = stmt->as.assignment.rvalue->as.array_init_expr.args;
          while (arg != NULL)
          {
            MiValue e = mi_eval_expression(table, arg);
            switch(e.type)
            {
              case MI_TYPE_BOOL:
                mi_array_add_int(array, (bool) e.as.number_value);
                break;
              case MI_TYPE_INT:
                mi_array_add_int(array, (int) e.as.number_value);
                break;
              case MI_TYPE_FLOAT:
                mi_array_add_float(array, (float) e.as.number_value);
                break;
              case MI_TYPE_STRING:
                mi_array_add_string(array, e.as.string_value);
                break;
              case MI_TYPE_ARRAY:
                mi_array_add_array(array, e.as.array_value);
                break;
              default:
                ASSERT_BREAK();
                break;
            }
            arg = arg->next;
          }

          value = mi_runtime_value_create_array(array);
        }
        else
        {
          value = mi_eval_expression(table, stmt->as.assignment.rvalue);
        }

        if (value.error_code != MI_ERROR_SUCCESS)
          return value;

        const char* identifier_name = stmt->as.assignment.lvalue->as.lvalue.identifier.str;
        if (stmt->as.assignment.lvalue->as.lvalue.type == LVALUE_ARRAY_ACCESS)
        {
          ASSERT(stmt->as.assignment.lvalue->as.lvalue.index_expression != NULL);
          MiValue index_expr_result = mi_eval_expression(table, stmt->as.assignment.lvalue->as.lvalue.index_expression);
          if (index_expr_result.type != MI_TYPE_INT)
          {
            index_expr_result.error_code = MI_ERROR_ARRAY_INDEX_TYPE;
            return index_expr_result;
          }

          MiSymbol* symbol = mi_symbol_table_get_variable(table, stmt->as.assignment.lvalue->as.lvalue.identifier.str);
          MiArray* array = symbol->as.variable.value.as.array_value;
          int index = (int) index_expr_result.as.number_value;
          if (index >= (int) array->size)
          {
            index_expr_result.error_code = MI_ERROR_ARRAY_INDEX_OUT_OF_BOUNDS;
            return index_expr_result;
          }

          if (value.type == MI_TYPE_BOOL)
          {
            array->elements[index].type = MI_TYPE_BOOL;
            array->elements[index].data.i = (bool) value.as.number_value;
            return mi_runtime_value_create_bool(value.as.number_value);
          }
          else if (value.type == MI_TYPE_INT)
          {
            array->elements[index].type = MI_TYPE_INT;
            array->elements[index].data.i = (int) value.as.number_value;
            return mi_runtime_value_create_int((int) value.as.number_value);
          }
          else if (value.type == MI_TYPE_FLOAT)
          {
            array->elements[index].type = MI_TYPE_FLOAT;
            array->elements[index].data.f = value.as.number_value;
            return mi_runtime_value_create_float(value.as.number_value);
          }
          else if (value.type == MI_TYPE_STRING)
          {
            array->elements[index].type = MI_TYPE_STRING;
            array->elements[index].data.s = value.as.string_value;
            return mi_runtime_value_create_string(value.as.string_value);
          }

          ASSERT_BREAK();
        }
        else
        {
          if (value.type == MI_TYPE_BOOL)
          {
            mi_symbol_table_set_variable_bool(table, identifier_name, value.as.number_value);
            return mi_runtime_value_create_bool(value.as.number_value);
          }
          else if (value.type == MI_TYPE_INT)
          {
            mi_symbol_table_set_variable_int(table, identifier_name, (int) value.as.number_value);
            return mi_runtime_value_create_int((int) value.as.number_value);
          }
          else if (value.type == MI_TYPE_FLOAT)
          {
            mi_symbol_table_set_variable_float(table, identifier_name, value.as.number_value);
            return mi_runtime_value_create_float(value.as.number_value);
          }
          else if (value.type == MI_TYPE_STRING)
          {
            mi_symbol_table_set_variable_string(table, identifier_name, value.as.string_value);
            return mi_runtime_value_create_string(value.as.string_value);
          }
          else if (value.type == MI_TYPE_ARRAY)
          {
            mi_symbol_table_set_variable_array(table, identifier_name, value.as.array_value);
            return value;
          }

          ASSERT_BREAK();
        }

        break;
      }
    case AST_STATEMENT_RETURN: 
      {
        //TODO: Implement AST_STATEMENT_RETURN
        return mi_runtime_value_create_void();
        break;
      }
    case AST_STATEMENT_IF: 
      {
        MiValue result = mi_runtime_value_create_void();
        MiValue condition = condition = mi_eval_expression(table, stmt->as.if_stmt.condition);
        ASSERT(condition.type == MI_TYPE_FLOAT || condition.type == MI_TYPE_INT || condition.type == MI_TYPE_BOOL);

        if (condition.as.number_value != 0) 
        {
          s_mi_symbol_table_scope_begin(table);
          result = mi_eval_statement_list(table, stmt->as.if_stmt.if_branch);
          s_mi_symbol_table_scope_end(table);
        }
        else if (stmt->as.if_stmt.else_branch != NULL)
        {
          s_mi_symbol_table_scope_begin(table);
          result = mi_eval_statement_list(table, stmt->as.if_stmt.else_branch);
          s_mi_symbol_table_scope_end(table);
        }

        return result;
        break;
      }
    case AST_STATEMENT_FOR: 
      {
        MiValue result = mi_runtime_value_create_void();
        s_mi_symbol_table_scope_begin(table);
        // Initialization
        MiValue init = mi_eval_statement(table, stmt->as.for_stmt.init);
        if (init.error_code != MI_ERROR_SUCCESS)
          return init;

        while(true)
        {
          // Condition
          MiValue condition = condition = mi_eval_expression(table, stmt->as.for_stmt.condition);
          ASSERT(condition.type == MI_TYPE_FLOAT || condition.type == MI_TYPE_INT || condition.type == MI_TYPE_BOOL);

          if (condition.error_code != MI_ERROR_SUCCESS)
          {
            result.error_code = condition.error_code;
            break;
          }

          if (condition.as.number_value == 0)
            break;

          MiValue v = mi_eval_statement_list(table, stmt->as.for_stmt.body);
          if (v.error_code != MI_ERROR_SUCCESS)
          {
            result.error_code = v.error_code;
            break;
          }
          
          if (v.type == MI_TYPE_INTERNAL_BREAK)
            break;

          // Update
          MiValue update = condition = mi_eval_statement(table, stmt->as.for_stmt.update);
          ASSERT(update.type == MI_TYPE_FLOAT || update.type == MI_TYPE_INT || update.type == MI_TYPE_BOOL);
          if (update.error_code != MI_ERROR_SUCCESS)
          {
            result.error_code = update.error_code;
            break;
          }
        }
        s_mi_symbol_table_scope_end(table);

        return result;

        break;
      }
    case AST_STATEMENT_WHILE: 
      {
        while (true)
        {
          // Eval Condition and break if false
          MiValue value = mi_eval_expression(table, stmt->as.while_stmt.condition);
          if(value.error_code != MI_ERROR_SUCCESS)
            return value;

          if (value.as.number_value == false)
            break;

          // Eval block
          ASTStatement* statement = stmt->as.while_stmt.body;
          s_mi_symbol_table_scope_begin(table);
          MiValue v = mi_eval_statement_list(table, statement);
          s_mi_symbol_table_scope_end(table);

          if (v.error_code != MI_ERROR_SUCCESS)
            return v;

          if (v.type == MI_TYPE_INTERNAL_BREAK)
            break;
        }

        break;
      }
    case AST_STATEMENT_FUNCTION_CALL: 
      return mi_eval_expression(table, stmt->as.expression);
      break;
    case AST_STATEMENT_FUNCTION_DECL: 
      {
        //TODO: implement function declarations
        break;
      }
    case AST_STATEMENT_BREAK: 
      return mi_ast_expression_create_break();
      break;
    default:
      break;
  }

  return mi_runtime_value_create_void();
}

MiValue mi_eval_statement_list(MiSymbolTable* table, ASTStatement* first_stmt)
{
  ASTStatement* statement = first_stmt;
  while(statement != NULL)
  {
    MiValue v = mi_eval_statement(table, statement);

    if (v.error_code != MI_ERROR_SUCCESS)
      return v;

    if (v.type == MI_TYPE_INTERNAL_BREAK)
      return v;

    statement = statement->next;
  }

  return mi_runtime_value_create_void();
}

int mi_eval_program(MiSymbolTable* table, ASTProgram* program) 
{
  MiValue last_value = {0};
  ASTStatement* statement = program->body;

  while(statement != NULL)
  {
    last_value = mi_eval_statement(table, statement);
    if (last_value.error_code != MI_ERROR_SUCCESS)
    {
      int code = last_value.error_code;
      log_error("Run-time error '%04d': %s.\n", code, mi_error_name(last_value.error_code));
      return code;
    }

    statement = statement->next;
  }

  if (last_value.type == EXPR_LITERAL_INT)
    return (int) last_value.as.number_value;

  return 0;
}

