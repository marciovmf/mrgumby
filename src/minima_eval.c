#include "common.h"
#include "minima_ast.h"
#include "minima_eval.h"


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
  };

  if (error >= 0 && error < MI_ERROR_COUNT_)
  {
    return error_name[error];
  }
  return "Unknown error";
}

MiValue mi_runtime_value_create_bool(bool value)
{
  return (MiValue){ .type = MI_VAL_BOOL, .as.number_value = value, .error_code = MI_ERROR_SUCCESS };
}

MiValue mi_runtime_value_create_int(int value)
{
  return (MiValue){ .type = MI_VAL_INT, .as.number_value = value, .error_code = MI_ERROR_SUCCESS };
}

MiValue mi_runtime_value_create_float(double value)
{
  return (MiValue){ .type = MI_VAL_FLOAT, .as.number_value = value, .error_code = MI_ERROR_SUCCESS};
}

MiValue mi_runtime_value_create_string(char* value)
{
  return (MiValue){ .type = MI_VAL_STRING, .as.string_value = value, .error_code = MI_ERROR_SUCCESS};
}

MiValue mi_runtime_value_create_void(void)
{
  return (MiValue){ .type = MI_VAL_VOID, .error_code = MI_ERROR_SUCCESS};
}

inline MiValue mi_runtime_value_create_error(MiError error)
{
  return (MiValue){ .type = MI_VAL_VOID, .error_code = error};
}

void mi_symbol_table_init(MiSymbolTable* table) 
{
  table->count = 0;
  table->scope = 0;
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

MiSymbol* mi_symbol_table_get_variable(MiSymbolTable* table, const char* identifier) 
{
  MiSymbol* variable = s_symbol_table_get_variable(table, identifier);
  if (variable == NULL)
    log_error("Requested uninitialized variable '%s'", identifier);

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

  log_warning("Requested unknown function '%s'", identifier);
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


//
// Evaluation fucntions
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
    case EXPR_FACTOR:
      {
        MiValue left = mi_eval_expression(table, expr->as.term_expr.left);
        MiValue right = mi_eval_expression(table, expr->as.term_expr.right);
        MiValueType resultType;

        if (left.type == MI_VAL_STRING || right.type == MI_VAL_STRING)
          resultType = MI_VAL_STRING;
        if (left.type == MI_VAL_FLOAT || right.type == MI_VAL_FLOAT)
          resultType = MI_VAL_FLOAT;
        else
          resultType = MI_VAL_INT;
        switch (expr->as.factor_expr.op) 
        {
          case OP_ADD:
            {
              if (resultType == MI_VAL_INT)
                return mi_runtime_value_create_int((int)(left.as.number_value + right.as.number_value));
              else if (resultType == MI_VAL_FLOAT)
                return mi_runtime_value_create_float(left.as.number_value + right.as.number_value);
              else if (resultType == MI_VAL_STRING)
                //TODO: String concatenation and conversion
                return mi_runtime_value_create_error(MI_ERROR_NOT_IMPLEMENTED);
              else
                ASSERT_BREAK();
            }
          case OP_SUBTRACT:
            {
              if (resultType == MI_VAL_INT)
                return mi_runtime_value_create_int((int)(left.as.number_value - right.as.number_value));
              else if (resultType == MI_VAL_FLOAT)
                return mi_runtime_value_create_float(left.as.number_value - right.as.number_value);
              else if (resultType == MI_VAL_STRING)
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
        MiValueType resultType;

        if (left.type == MI_VAL_STRING || right.type == MI_VAL_STRING)
          resultType = MI_VAL_STRING;
        if (left.type == MI_VAL_FLOAT || right.type == MI_VAL_FLOAT)
          resultType = MI_VAL_FLOAT;
        else
          resultType = MI_VAL_INT;
        switch (expr->as.term_expr.op) 
        {
          case OP_MULTIPLY:
            {
              if (resultType == MI_VAL_INT)
                return mi_runtime_value_create_int((int)(left.as.number_value * right.as.number_value));
              else if (resultType == MI_VAL_FLOAT)
                return mi_runtime_value_create_float(left.as.number_value * right.as.number_value);
              else if (resultType == MI_VAL_STRING)
                //TODO: String concatenation and conversion 
                return mi_runtime_value_create_error(MI_ERROR_NOT_IMPLEMENTED);
              else
                ASSERT_BREAK();
            }
          case OP_DIVIDE:
            {
              if (resultType != EXPR_LITERAL_STRING && right.as.number_value == 0.0)
                return mi_runtime_value_create_error(MI_ERROR_DIVIDE_BY_ZERO);

              if (resultType == MI_VAL_INT)
                return mi_runtime_value_create_int((int)(left.as.number_value / right.as.number_value));
              else if (resultType == MI_VAL_FLOAT)
                return mi_runtime_value_create_float(left.as.number_value / right.as.number_value);
              else if (resultType == MI_VAL_STRING)
                //TODO: String concatenation and conversion 
                return mi_runtime_value_create_error(MI_ERROR_NOT_IMPLEMENTED);
              else
                ASSERT_BREAK();
            }
          case OP_MOD:
            {
              if (resultType == MI_VAL_STRING)
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
        return lvalue->as.variable.value;
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
        MiValue value = mi_eval_expression(table, stmt->as.assignment.rvalue);
        if (value.error_code != MI_ERROR_SUCCESS)
          return value;

        const char* identifier_name = stmt->as.assignment.lvalue->as.lvalue.identifier.str;
        if (value.type == MI_VAL_BOOL)
        {
          mi_symbol_table_set_variable_bool(table, identifier_name, value.as.number_value);
          return mi_runtime_value_create_bool(value.as.number_value);
        }
        if (value.type == MI_VAL_INT)
        {
          mi_symbol_table_set_variable_int(table, identifier_name, (int) value.as.number_value);
          return mi_runtime_value_create_int((int) value.as.number_value);
        }
        else if (value.type == MI_VAL_FLOAT)
        {
          mi_symbol_table_set_variable_float(table, identifier_name, value.as.number_value);
          return mi_runtime_value_create_float(value.as.number_value);
        }
        else if (value.type == MI_VAL_STRING)
        {
          mi_symbol_table_set_variable_string(table, identifier_name, value.as.string_value);
          return mi_runtime_value_create_string(value.as.string_value);
        }
        else
        {
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
        MiValue condition = condition = mi_eval_expression(table, stmt->as.if_stmt.condition);
        ASSERT(condition.type == MI_VAL_FLOAT || condition.type == MI_VAL_INT || condition.type == MI_VAL_BOOL);

        if (condition.as.number_value != 0) 
        {
          s_mi_symbol_table_scope_begin(table);
          mi_eval_statement_list(table, stmt->as.if_stmt.if_branch);
          s_mi_symbol_table_scope_end(table);
        }
        else if (stmt->as.if_stmt.else_branch != NULL)
        {
          table->scope++;
          mi_eval_statement_list(table, stmt->as.if_stmt.else_branch);
          table->scope--;
        }
        return mi_runtime_value_create_void();
        break;
      }
    case AST_STATEMENT_FOR: 
      {
        //TODO: implement for loops
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
          while(statement != NULL)
          {
            mi_eval_statement(table, statement);
            statement = statement->next;
          }
          s_mi_symbol_table_scope_end(table);
        }

        break;
      }
    case AST_STATEMENT_FUNCTION_CALL: 
      MiSymbol* symbol = mi_symbol_table_get_function(table, stmt->as.expression->as.func_call_expr.identifier.str);
      MiValue arg0 = mi_eval_expression(table, stmt->as.expression->as.func_call_expr.args);
      symbol->as.function.function_ptr(1, &arg0);
      break;
    case AST_STATEMENT_FUNCTION_DECL: 
      {
        //TODO: implement function declarations
        break;
      }
    case AST_STATEMENT_BREAK: 
      {
        //TODO: implement break statement
        break;
      }
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
    mi_eval_statement(table, statement);
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
