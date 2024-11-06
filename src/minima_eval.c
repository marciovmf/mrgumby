#include "common.h"
#include "minima_ast.h"
#include "minima_eval.h"


//
// Runtime expression value
//

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
}

inline MiSymbol* s_symbol_table_get_variable(MiSymbolTable* table, const char* identifier) 
{
  for (int i = 0; i < table->count; i++) 
  {
    if (strcmp(table->entry[i].identifier.str, identifier) == 0 && table->entry[i].type == MI_SYMBOL_VARIABLE) 
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
    log_warning("Requested uninitialized variable '%s'", identifier);

  return variable;
}

MiSymbol* mi_symbol_table_get_function(MiSymbolTable* table, const char* identifier) 
{
  for (int i = 0; i < table->count; i++) 
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
        ASTExpressionType resultType;

        if (left.type == EXPR_LITERAL_STRING || right.type == EXPR_LITERAL_STRING)
          resultType = EXPR_LITERAL_STRING;
        if (left.type == EXPR_LITERAL_FLOAT || right.type == EXPR_LITERAL_FLOAT)
          resultType = EXPR_LITERAL_FLOAT;
        else
          resultType = EXPR_LITERAL_INT;
        switch (expr->as.factor_expr.op) 
        {
          case OP_ADD:
            {
              if (resultType == EXPR_LITERAL_INT)
                return mi_runtime_value_create_int((int)(left.as.number_value + right.as.number_value));
              else if (resultType == EXPR_LITERAL_FLOAT)
                return mi_runtime_value_create_float((int)left.as.number_value + right.as.number_value);
              else if (resultType == EXPR_LITERAL_STRING)
                //TODO: String concatenation and conversion
                return mi_runtime_value_create_error(MI_ERROR_NOT_IMPLEMENTED);
              else
                ASSERT_BREAK();
            }
          case OP_SUBTRACT:
            {
              if (resultType == EXPR_LITERAL_INT)
                return mi_runtime_value_create_int((int)(left.as.number_value - right.as.number_value));
              else if (resultType == EXPR_LITERAL_FLOAT)
                return mi_runtime_value_create_float(left.as.number_value - right.as.number_value);
              else if (resultType == EXPR_LITERAL_STRING)
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
        ASTExpressionType resultType;

        if (left.type == EXPR_LITERAL_STRING || right.type == EXPR_LITERAL_STRING)
          resultType = EXPR_LITERAL_STRING;
        if (left.type == EXPR_LITERAL_FLOAT || right.type == EXPR_LITERAL_FLOAT)
          resultType = EXPR_LITERAL_FLOAT;
        else
          resultType = EXPR_LITERAL_INT;
        switch (expr->as.term_expr.op) 
        {
          case OP_MULTIPLY:
            {
              if (resultType == EXPR_LITERAL_INT)
                return mi_runtime_value_create_int((int)(left.as.number_value * right.as.number_value));
              else if (resultType == EXPR_LITERAL_FLOAT)
                return mi_runtime_value_create_float(left.as.number_value * right.as.number_value);
              else if (resultType == EXPR_LITERAL_STRING)
                //TODO: String concatenation and conversion 
                return mi_runtime_value_create_error(MI_ERROR_NOT_IMPLEMENTED);
              else
                ASSERT_BREAK();
            }
          case OP_DIVIDE:
            {
              if (resultType != EXPR_LITERAL_STRING && right.as.number_value == 0.0)
                return mi_runtime_value_create_error(MI_ERROR_DIVIDE_BY_ZERO);

              if (resultType == EXPR_LITERAL_INT)
                return mi_runtime_value_create_int((int)(left.as.number_value / right.as.number_value));
              else if (resultType == EXPR_LITERAL_FLOAT)
                return mi_runtime_value_create_float(left.as.number_value / right.as.number_value);
              else if (resultType == EXPR_LITERAL_STRING)
                //TODO: String concatenation and conversion 
                return mi_runtime_value_create_error(MI_ERROR_NOT_IMPLEMENTED);
              else
                ASSERT_BREAK();
            }
          case OP_MOD:
            {
              if (resultType == EXPR_LITERAL_STRING)
                return mi_runtime_value_create_error(MI_ERROR_UNSUPPOMiED_OPERATION);
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
        return mi_symbol_table_get_variable(table, expr->as.identifier.str)->as.variable.value;
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
    case AST_STATEMENT_ASSIGNMENT: 
      {
        MiValue value = mi_eval_expression(table, stmt->as.assignment.expression);
        ASSERT(value.error_code == MI_ERROR_SUCCESS);

        if (value.type == MI_VAL_BOOL)
        {
          mi_symbol_table_set_variable_bool(table, stmt->as.assignment.identifier.str, value.as.number_value);
          return mi_runtime_value_create_bool(value.as.number_value);
        }
        if (value.type == MI_VAL_INT)
        {
          mi_symbol_table_set_variable_int(table, stmt->as.assignment.identifier.str, (int) value.as.number_value);
          return mi_runtime_value_create_int((int) value.as.number_value);
        }
        else if (value.type == MI_VAL_FLOAT)
        {
          mi_symbol_table_set_variable_float(table, stmt->as.assignment.identifier.str, value.as.number_value);
          return mi_runtime_value_create_float(value.as.number_value);
        }
        else if (value.type == MI_VAL_STRING)
        {
          mi_symbol_table_set_variable_string(table, stmt->as.assignment.identifier.str, value.as.string_value);
          return mi_runtime_value_create_string(value.as.string_value);
        }
        else
        {
          ASSERT_BREAK();
        }
        break;
      }
      //case AST_STATEMENT_PRINT: 
      //  {
      //    MiValue value = eval_expression(table, stmt->as.print_expr);
      //    if (value.type == EXPR_LITERAL_INT)
      //    {
      //      printf("%d\n", (int) value.as.number_value);
      //    }
      //    else if (value.type == EXPR_LITERAL_INT)
      //    {
      //      printf("%f\n", value.as.number_value);
      //    }
      //    else if (value.type == EXPR_LITERAL_STRING)
      //    {
      //      printf("%s\n", value.as.string_value);
      //    }
      //    else
      //    {
      //      ASSERT_BREAK();
      //    }

      //    return mi_runtime_value_create_void();
      //  }
    case AST_STATEMENT_RETURN: 
      {
        //TODO: Implement AST_STATEMENT_RETURN
        return mi_runtime_value_create_void();
        break;
      }
    case AST_STATEMENT_IF: 
      {
        MiValue condition = condition = mi_eval_expression(table, stmt->as.if_stmt.condition);
        ASSERT(condition.error_code == MI_ERROR_SUCCESS && (condition.type == MI_VAL_FLOAT || condition.type == MI_VAL_INT || condition.type == MI_VAL_BOOL ));

        if (condition.as.number_value != 0) 
        {
          mi_eval_statement(table, stmt->as.if_stmt.if_branch);
        } else if (stmt->as.if_stmt.else_branch != NULL)
        {
          mi_eval_statement(table, stmt->as.if_stmt.else_branch);
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
          ASSERT(value.error_code == MI_ERROR_SUCCESS);
          if (value.as.number_value == false)
            break;

          // Eval block
          ASTStatement* statement = stmt->as.while_stmt.body;
          while(statement != NULL)
          {
            mi_eval_statement(table, statement);
            statement = statement->next;
          }
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

int mi_eval_program(MiSymbolTable* table, ASTProgram* program) 
{
  MiValue last_value = {0};
  ASTStatement* statement = program->body;

  while(statement != NULL)
  {
    last_value = mi_eval_statement(table, statement);
    if (last_value.error_code != MI_ERROR_SUCCESS)
      return (int) last_value.error_code;

    statement = statement->next;
  }

  if (last_value.type == EXPR_LITERAL_INT)
    return (int) last_value.as.number_value;

  return 0;
}
