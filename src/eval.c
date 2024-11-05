#include "ast.h"
#include "common.h"
#include "eval.h"

//
// Runtime expression value
//

RTValue runtime_value_create_bool(bool value)
{
  return (RTValue){ .type = RT_VAL_BOOL, .as.number_value = value, .error_code = RT_ERROR_SUCCESS };
}

RTValue runtime_value_create_int(int value)
{
  return (RTValue){ .type = RT_VAL_INT, .as.number_value = value, .error_code = RT_ERROR_SUCCESS };
}

RTValue runtime_value_create_float(double value)
{
  return (RTValue){ .type = RT_VAL_FLOAT, .as.number_value = value, .error_code = RT_ERROR_SUCCESS};
}

RTValue runtime_value_create_string(char* value)
{
  return (RTValue){ .type = RT_VAL_STRING, .as.string_value = value, .error_code = RT_ERROR_SUCCESS};
}

RTValue runtime_value_create_void(void)
{
  return (RTValue){ .type = RT_VAL_VOID, .error_code = RT_ERROR_SUCCESS};
}

inline RTValue runtime_value_create_error(RTError error)
{
  return (RTValue){ .type = RT_VAL_VOID, .error_code = error};
}

//
// Symbol table functions
//

void symbol_table_init(RTSymbolTable* table) 
{
  table->count = 0;
}

RTSymbol* symbol_table_get_variable(RTSymbolTable* table, const char* identifier) 
{
  for (int i = 0; i < table->count; i++) 
  {
    if (strcmp(table->entry[i].identifier.str, identifier) == 0 && table->entry[i].type == RT_SYMBOL_VARIABLE) 
    {
      return &table->entry[i];
    }
  }

  log_warning("Requested uninitialized variable '%s'", identifier);
  return 0;
}

RTSymbol* symbol_table_get_function(RTSymbolTable* table, const char* identifier) 
{
  for (int i = 0; i < table->count; i++) 
  {
    if (strcmp(table->entry[i].identifier.str, identifier) == 0 && table->entry[i].type == RT_SYMBOL_FUNCTION) 
    {
      return &table->entry[i];
    }
  }

  log_warning("Requested unknown function '%s'", identifier);
  return 0;
}

void symbol_table_set_variable_bool(RTSymbolTable* table, const char* identifier, bool value)
{
  RTSymbol* variable = symbol_table_get_variable(table, identifier);
  if (variable != NULL)
  {
      variable->as.variable.value = runtime_value_create_bool(value);
  }
  else
  {
    smallstr(&table->entry[table->count].identifier, identifier);
    table->entry[table->count].type = RT_SYMBOL_VARIABLE;
    table->entry[table->count].as.variable.name = table->entry[table->count].identifier.str;
    table->entry[table->count].as.variable.value = runtime_value_create_bool(value);
    table->count++;
  }
}

void symbol_table_set_variable_int(RTSymbolTable* table, const char* identifier, int value) 
{
  RTSymbol* variable = symbol_table_get_variable(table, identifier);
  if (variable != NULL)
  {
      variable->as.variable.value = runtime_value_create_int(value);
  }
  else
  {
    smallstr(&table->entry[table->count].identifier, identifier);
    table->entry[table->count].type = RT_SYMBOL_VARIABLE;
    table->entry[table->count].as.variable.name = table->entry[table->count].identifier.str;
    table->entry[table->count].as.variable.value = runtime_value_create_int(value);
    table->count++;
  }
}

void symbol_table_set_variable_float(RTSymbolTable* table, const char* identifier, double value) 
{
  RTSymbol* variable = symbol_table_get_variable(table, identifier);
  if (variable != NULL)
  {
      variable->as.variable.value = runtime_value_create_float(value);
  }
  else
  {
    smallstr(&table->entry[table->count].identifier, identifier);
    table->entry[table->count].type = RT_SYMBOL_VARIABLE;
    table->entry[table->count].as.variable.name = table->entry[table->count].identifier.str;
    table->entry[table->count].as.variable.value = runtime_value_create_float(value);
    table->count++;
  }
}

void symbol_table_set_variable_string(RTSymbolTable* table, const char* identifier, char* value) 
{
  RTSymbol* variable = symbol_table_get_variable(table, identifier);
  if (variable != NULL)
  {
      variable->as.variable.value = runtime_value_create_string(value);
  }
  else
  {
    smallstr(&table->entry[table->count].identifier, identifier);
    table->entry[table->count].type = RT_SYMBOL_VARIABLE;
    table->entry[table->count].as.variable.name = table->entry[table->count].identifier.str;
    table->entry[table->count].as.variable.value = runtime_value_create_string(value);
    table->count++;
  }
}


//
// Evaluation fucntions
//

RTValue eval_expression(RTSymbolTable* table, ASTExpression* expr) 
{
  switch (expr->type) 
  {
    case EXPR_LITERAL_BOOL:
      return runtime_value_create_bool(((int) expr->as.number_literal) != 0);
    case EXPR_LITERAL_INT:
      return  runtime_value_create_int((int) expr->as.number_literal);
    case EXPR_LITERAL_FLOAT:
      return runtime_value_create_float(expr->as.number_literal);
    case EXPR_LITERAL_STRING:
      return runtime_value_create_string(expr->as.string_literal);
    case EXPR_FACTOR:
      {
        RTValue left = eval_expression(table, expr->as.term_expr.left);
        RTValue right = eval_expression(table, expr->as.term_expr.right);
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
                return runtime_value_create_int((int)(left.as.number_value + right.as.number_value));
              else if (resultType == EXPR_LITERAL_FLOAT)
                return runtime_value_create_float((int)left.as.number_value + right.as.number_value);
              else if (resultType == EXPR_LITERAL_STRING)
                //TODO: String concatenation and conversion
                return runtime_value_create_error(RT_ERROR_NOT_IMPLEMENTED);
              else
                ASSERT_BREAK();
            }
          case OP_SUBTRACT:
            {
              if (resultType == EXPR_LITERAL_INT)
                return runtime_value_create_int((int)(left.as.number_value - right.as.number_value));
              else if (resultType == EXPR_LITERAL_FLOAT)
                return runtime_value_create_float(left.as.number_value - right.as.number_value);
              else if (resultType == EXPR_LITERAL_STRING)
                //TODO: String concatenation and conversion
                return runtime_value_create_error(RT_ERROR_NOT_IMPLEMENTED);
              else
                ASSERT_BREAK();
            }
          default:
            log_and_break("Unknown binary operator for FACTOR");
            return runtime_value_create_void();
            break;
        }
      }
      break;
    case EXPR_TERM: 
      {
        RTValue left =  eval_expression(table, expr->as.factor_expr.left);
        RTValue right = eval_expression(table, expr->as.factor_expr.right);
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
                return runtime_value_create_int((int)(left.as.number_value * right.as.number_value));
              else if (resultType == EXPR_LITERAL_FLOAT)
                return runtime_value_create_float(left.as.number_value * right.as.number_value);
              else if (resultType == EXPR_LITERAL_STRING)
                //TODO: String concatenation and conversion 
                return runtime_value_create_error(RT_ERROR_NOT_IMPLEMENTED);
              else
                ASSERT_BREAK();
            }
          case OP_DIVIDE:
            {
              if (resultType != EXPR_LITERAL_STRING && right.as.number_value == 0.0)
                return runtime_value_create_error(RT_ERROR_DIVIDE_BY_ZERO);

              if (resultType == EXPR_LITERAL_INT)
                return runtime_value_create_int((int)(left.as.number_value / right.as.number_value));
              else if (resultType == EXPR_LITERAL_FLOAT)
                return runtime_value_create_float(left.as.number_value / right.as.number_value);
              else if (resultType == EXPR_LITERAL_STRING)
                //TODO: String concatenation and conversion 
                return runtime_value_create_error(RT_ERROR_NOT_IMPLEMENTED);
              else
                ASSERT_BREAK();
            }
          case OP_MOD:
            {
              if (resultType == EXPR_LITERAL_STRING)
                return runtime_value_create_error(RT_ERROR_UNSUPPORTED_OPERATION);
              else
                return runtime_value_create_int(((int)left.as.number_value % (int) right.as.number_value));
            }
          default:
            log_and_break("Unknown binary operator for TERM");
            return runtime_value_create_void();
            break;
        }

      } break;
    case EXPR_LVALUE:
      {
        return symbol_table_get_variable(table, expr->as.identifier.str)->as.variable.value;
      }
    case EXPR_UNARY:
      {
        RTValue value = eval_expression(table, expr->as.unary_expr.expression);
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
        RTValue left = eval_expression(table, expr->as.term_expr.left);
        RTValue right = eval_expression(table, expr->as.term_expr.right);

        switch (expr->as.comparison_expr.op)
        {
          case OP_LT:
            return runtime_value_create_bool(left.as.number_value < right.as.number_value);
            break;
          case OP_GT:
            return runtime_value_create_bool(left.as.number_value > right.as.number_value);
            break;
          case OP_LTE:
            return runtime_value_create_bool(left.as.number_value <= right.as.number_value);
            break;
          case OP_GTE:
            return runtime_value_create_bool(left.as.number_value <= right.as.number_value);
            break;
          case OP_EQ:
            return runtime_value_create_bool(left.as.number_value == right.as.number_value);
            break;
          case OP_NEQ:
            return runtime_value_create_bool(left.as.number_value != right.as.number_value);
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
  return runtime_value_create_void();
}

RTValue eval_statement(RTSymbolTable* table, ASTStatement* stmt) 
{
  switch (stmt->type) 
  {
    case AST_STATEMENT_ASSIGNMENT: 
      {
        RTValue value = eval_expression(table, stmt->as.assignment.expression);
        ASSERT(value.error_code == RT_ERROR_SUCCESS);

        if (value.type == RT_VAL_BOOL)
        {
          symbol_table_set_variable_bool(table, stmt->as.assignment.identifier.str, value.as.number_value);
          return runtime_value_create_bool(value.as.number_value);
        }
        if (value.type == RT_VAL_INT)
        {
          symbol_table_set_variable_int(table, stmt->as.assignment.identifier.str, (int) value.as.number_value);
          return runtime_value_create_int((int) value.as.number_value);
        }
        else if (value.type == RT_VAL_FLOAT)
        {
          symbol_table_set_variable_float(table, stmt->as.assignment.identifier.str, value.as.number_value);
          return runtime_value_create_float(value.as.number_value);
        }
        else if (value.type == RT_VAL_STRING)
        {
          symbol_table_set_variable_string(table, stmt->as.assignment.identifier.str, value.as.string_value);
          return runtime_value_create_string(value.as.string_value);
        }
        else
        {
          ASSERT_BREAK();
        }
        break;
      }
      //case AST_STATEMENT_PRINT: 
      //  {
      //    RTValue value = eval_expression(table, stmt->as.print_expr);
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

      //    return runtime_value_create_void();
      //  }
    case AST_STATEMENT_RETURN: 
      {
        //TODO: Implement AST_STATEMENT_RETURN
        return runtime_value_create_void();
        break;
      }
    case AST_STATEMENT_IF: 
      {
        RTValue condition = condition = eval_expression(table, stmt->as.if_stmt.condition);
        ASSERT(condition.error_code == RT_ERROR_SUCCESS && (condition.type == RT_VAL_FLOAT || condition.type == RT_VAL_INT || condition.type == RT_VAL_BOOL ));

        if (condition.as.number_value != 0) 
        {
          eval_statement(table, stmt->as.if_stmt.if_branch);
        } else if (stmt->as.if_stmt.else_branch != NULL)
        {
          eval_statement(table, stmt->as.if_stmt.else_branch);
        }
        return runtime_value_create_void();
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
          RTValue value = eval_expression(table, stmt->as.while_stmt.condition);
          ASSERT(value.error_code == RT_ERROR_SUCCESS);
          if (value.as.number_value == false)
            break;

          // Eval block
          ASTStatement* statement = stmt->as.while_stmt.body;
          while(statement != NULL)
          {
            eval_statement(table, statement);
            statement = statement->next;
          }
        }

        break;
      }
    case AST_STATEMENT_FUNCTION_CALL: 
      RTSymbol* symbol = symbol_table_get_function(table, stmt->as.expression->as.func_call_expr.identifier.str);
      RTValue arg0 = eval_expression(table, stmt->as.expression->as.func_call_expr.args);
      symbol->as.function.function_ptr(1, &arg0);
      break;
    case AST_STATEMENT_FUNCTION_DECL: 
      {
        //TODO: implement function declarations
        break;
      }
    case AST_STATEMENT_INPUT: 
      {
        //TODO: implement input statement
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

  return runtime_value_create_void();
}

int eval_program(RTSymbolTable* table, ASTProgram* program) 
{
  RTValue last_value = {0};
  ASTStatement* statement = program->body;

  while(statement != NULL)
  {
    last_value = eval_statement(table, statement);
    if (last_value.error_code != RT_ERROR_SUCCESS)
      return (int) last_value.error_code;

    statement = statement->next;
  }

  if (last_value.type == EXPR_LITERAL_INT)
    return (int) last_value.as.number_value;

  return 0;
}
