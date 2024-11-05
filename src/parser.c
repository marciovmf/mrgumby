/**
 * @file parser.c
 * @brief Defines the lexer and parser for tokenizing and parsing source code.
 *
 * This header contains structures and functions for the lexer and parser, 
 * including token definitions, lexer operations, and parsing functions.
 * 
 * @author marciofmv
 */
#include "parser.h"
#include "common.h"
#include "ast.h"

#define report_error(lexer, message) log_error("Syntax error at line %d, column %d: %s\n",\
    lexer->line, lexer->column, message);

#define report_error_unexpected_token(lexer, token_type) log_error("Sytax error at %d, %d: Unexpected '%s' token \n",\
    lexer->line, lexer->column, token_get_name(token_type))

const char* token_get_name(TokenType token)
{
  static const char *tokenNames[TOKEN_COUNT_] = {
    [TOKEN_ERROR]           = "Invalid",
    [TOKEN_LOGICAL_AND]     = "Logical AND operator",
    [TOKEN_LOGICAL_OR]      = "Logical OR operator",
    [TOKEN_OP_ASSIGN]       = "Assignment operator",
    [TOKEN_OP_EQ]           = "Equality operator",
    [TOKEN_OP_NEQ]          = "Inequality operator",
    [TOKEN_OP_LT]           = "Less-than operator",
    [TOKEN_OP_LTE]          = "Less-than-or-equal-to operator",
    [TOKEN_OP_GT]           = "Greater-than operator",
    [TOKEN_OP_GTE]          = "Greater-than-or-equal-to operator",
    [TOKEN_OPEN_PAREN]      = "Open parenthesis",
    [TOKEN_CLOSE_PAREN]     = "Close parenthesis",
    [TOKEN_OPEN_BRACE]      = "Open brace",
    [TOKEN_CLOSE_BRACE]     = "Close brace",
    [TOKEN_OPEN_BACKET]     = "Open bracket",
    [TOKEN_CLOSE_BRACKET]   = "Close bracket",
    [TOKEN_ASTERISK]        = "Multiplication operator",
    [TOKEN_SLASH]           = "Division operator",
    [TOKEN_PERCENT]         = "Modulus operator",
    [TOKEN_COMMA]           = "Comma",
    [TOKEN_SEMICOLON]       = "Semicolon",
    [TOKEN_DOT]             = "Dot",
    [TOKEN_EXCLAMATION]     = "Logical not operator",
    [TOKEN_PLUS]            = "Sum operator",
    [TOKEN_MINUS]           = "Subtraction operator",
    [TOKEN_IF]              = "if statement",
    [TOKEN_ELSE]            = "else statement",
    [TOKEN_FOR]             = "for statement",
    [TOKEN_RETURN]          = "return satement",
    [TOKEN_INCLUDE]         = "Include directive",
    [TOKEN_IDENTIFIER]      = "Identifier",
    [TOKEN_LITERAL_INT]     = "Integer literal",
    [TOKEN_LITERAL_FLOAT]   = "Floating-point literal",
    [TOKEN_LITERAL_STRING]  = "String literal",
    [TOKEN_EOF]             = "end of file"
  };

  if (token >= 0 && token < TOKEN_COUNT_)
  {
    return tokenNames[token];
  }
  return tokenNames[TOKEN_ERROR];
}


#define lexer_get_next_token(lexer) lexer_get_next_token_(lexer, false)
Token lexer_get_next_token_(Lexer *lexer, bool suppress_errors);
static ASTExpression* parse_term(Lexer* lexer);
static ASTExpression* parse_expression(Lexer* lexer);
static ASTStatement* parse_statement(Lexer *lexer);
static ASTStatement* parse_statement_list(Lexer* lexer);


static void lexer_advance(Lexer *lexer)
{
  if (lexer->current_char == 0)
    return;

  if (lexer->current_char == '\n')
  {
    lexer->line++;
    lexer->column = 1;
  } else
  {
    lexer->column++;
  }
  lexer->position++;
  lexer->current_char = lexer->buffer[lexer->position];
  lexer->next_char = lexer->current_char != 0 ? lexer->buffer[lexer->position + 1] : 0;
}


static Token lexer_look_ahead(Lexer* lexer)
{
  Lexer chekpoint = *lexer;
  Token t = lexer_get_next_token_(lexer, true);
  *lexer = chekpoint;
  return t;
}


static void lexer_look_ahead_2(Lexer* lexer, Token* token1, Token* token2)
{
  Lexer chekpoint = *lexer;
  *token1 = lexer_get_next_token_(lexer, true);
  *token2 = lexer_get_next_token_(lexer, true);
  *lexer = chekpoint;
}


static void lexer_skip_until_next_line(Lexer *lexer)
{
  while (lexer->current_char != '\n' && lexer->current_char != '\0')
  {
    lexer_advance(lexer);
  }
}


static void lexer_skip_whitespace(Lexer *lexer)
{
  while (isspace(lexer->current_char) || lexer->current_char == '#')
  {
    if (lexer->current_char == '#')
    {
      lexer_skip_until_next_line(lexer);  // Skip comment line
    } else
    {
      lexer_advance(lexer);  // Skip whitespace
    }
  }
}


static Token lexer_get_identifier(Lexer *lexer)
{

  Token token;
  int i = 0;

  while (isalpha(lexer->current_char) || lexer->current_char == '_')
  {
    token.value[i++] = lexer->current_char;
    lexer_advance(lexer);
  }
  token.value[i] = '\0';

  if (strcmp(token.value, "if") == 0) token.type = TOKEN_IF;
  else if (strcmp(token.value, "else") == 0) token.type = TOKEN_ELSE;
  else if (strcmp(token.value, "for") == 0) token.type = TOKEN_FOR;
  else if (strcmp(token.value, "include") == 0) token.type = TOKEN_INCLUDE;
  else if (strcmp(token.value, "return") == 0) token.type = TOKEN_RETURN;
  else if (strcmp(token.value, "while") == 0) token.type = TOKEN_WHILE;
  else if (strcmp(token.value, "true") == 0) token.type = TOKEN_LITERAL_BOOL;
  else if (strcmp(token.value, "false") == 0) token.type = TOKEN_LITERAL_BOOL;
  //else if (strcmp(token.value, "function") == 0) token.type = TOKEN_FUNCTION;
  //else if (strcmp(token.value, "endwhile") == 0) token.type = TOKEN_ENDWHILE;
  else token.type = TOKEN_IDENTIFIER;

  return token;
}


static Token lexer_get_literal_string(Lexer *lexer)
{
  Token out;

  int i = 0;
  // Expecting a starting quote
  if (lexer->current_char == '"')
  {
    lexer_advance(lexer);  // Move past the opening quote
    while (lexer->current_char != '"' && lexer->current_char != '\0')
    {
      out.value[i++] = lexer->current_char;
      lexer_advance(lexer);
    }
    out.value[i] = '\0';  // Null terminate the string
    if (lexer->current_char == '"')
    {
      lexer_advance(lexer);  // Move past the closing quote
      out.type = TOKEN_LITERAL_STRING;    
    } else
    {
      out.type = TOKEN_ERROR;
    }
  } else
  {
    report_error(lexer, "Expected opening quote for string");
    out.type = TOKEN_ERROR;
  }

  return out;
}


static bool lexer_skip_token(Lexer* lexer, TokenType expected_type)
{
  Token t = lexer_get_next_token(lexer);
  bool result = (t.type == expected_type);
  if (!result)
     log_error("Sytax error at %d, %d: Expecting '%s' but '%s' found. \n",
         lexer->line, lexer->column, token_get_name(expected_type), token_get_name(t.type));
  return true;
}


Token lexer_get_next_token_(Lexer *lexer, bool suppress_errors)
{
  Token token;
  lexer_skip_whitespace(lexer);

  if (lexer->current_char == '"')
  {
    return lexer_get_literal_string(lexer);
  }

  if (isalpha(lexer->current_char))
  {
    return lexer_get_identifier(lexer);
  } 
  else if (isdigit(lexer->current_char))
  {
    int i = 0;
    while (isdigit(lexer->current_char))
    {
      token.value[i++] = lexer->current_char;
      lexer_advance(lexer);
    }
    token.value[i] = '\0';
    token.type = TOKEN_LITERAL_INT;
    return token;
  }
  else if (lexer->current_char == '&' && lexer->next_char == '&')
  {
    token.type = TOKEN_LOGICAL_AND;
    token.value[0] = lexer->current_char;
    token.value[1] = lexer->next_char;
    token.value[3] = '\0';
    lexer_advance(lexer);
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '|' && lexer->next_char == '|')
  {
    token.type = TOKEN_LOGICAL_OR;
    token.value[0] = lexer->current_char;
    token.value[1] = lexer->next_char;
    token.value[3] = '\0';
    lexer_advance(lexer);
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '>' && lexer->next_char == '=')
  {
    token.type = TOKEN_OP_GTE;
    token.value[0] = lexer->current_char;
    token.value[1] = lexer->next_char;
    token.value[3] = '\0';
    lexer_advance(lexer);
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '<' && lexer->next_char == '=')
  {
    token.type = TOKEN_OP_LTE;
    token.value[0] = lexer->current_char;
    token.value[1] = lexer->next_char;
    token.value[3] = '\0';
    lexer_advance(lexer);
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '=' && lexer->next_char == '=')
  {
    token.type = TOKEN_OP_EQ;
    token.value[0] = lexer->current_char;
    token.value[1] = lexer->next_char;
    token.value[3] = '\0';
    lexer_advance(lexer);
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '!' && lexer->next_char == '=')
  {
    token.type = TOKEN_OP_NEQ;
    token.value[0] = lexer->current_char;
    token.value[1] = lexer->next_char;
    token.value[2] = '\0';
    lexer_advance(lexer);
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '>')
  {
    token.type = TOKEN_OP_GT;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '<')
  {
    token.type = TOKEN_OP_LT;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '=')
  {
    token.type = TOKEN_OP_ASSIGN;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '(')
  {
    token.type = TOKEN_OPEN_PAREN;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == ')')
  {
    token.type = TOKEN_CLOSE_PAREN;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '+')
  {
    token.type = TOKEN_PLUS;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '-')
  {
    token.type = TOKEN_MINUS;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '*')
  {
    token.type = TOKEN_ASTERISK;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '/')
  {
    token.type = TOKEN_SLASH;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '%')
  {
    token.type = TOKEN_PERCENT;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == ',')
  {
    token.type = TOKEN_COMMA;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == ';')
  {
    token.type = TOKEN_SEMICOLON;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '.')
  {
    token.type = TOKEN_DOT;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '!')
  {
    token.type = TOKEN_EXCLAMATION;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '{')
  {
    token.type = TOKEN_OPEN_BRACE;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '}')
  {
    token.type = TOKEN_CLOSE_BRACE;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '\0')
  {
    token.type = TOKEN_EOF;
    token.value[0] = '\0';
  }
  else
  {
    if (suppress_errors == false)
      log_error("Unexpected character at line %d, column %d: '%c'\n", lexer->line, lexer->column, lexer->current_char);
    token.type = TOKEN_ERROR;
  }

  return token;
}


static bool lexer_require_token(Lexer* lexer, TokenType expected_type, Token* out)
{
  *out = lexer_get_next_token(lexer);
  bool result = (out->type == expected_type);
  if (!result)
    report_error_unexpected_token(lexer, out->type);
  return true;
}



//
// Parser
//


/*
* <ArgList> -> [ <Expression> ( "," <Expression> )* ]
*/
static ASTExpression* parse_arg_list(Lexer* lexer)
{
  ASTExpression* arg_list = parse_expression(lexer);
  ASTExpression* args = arg_list;

  while (args != NULL)
  {
    Token look_ahead_token = lexer_look_ahead(lexer);
    if (look_ahead_token.type != TOKEN_COMMA)
      break;

    lexer_skip_token(lexer, TOKEN_COMMA);
    args->next = parse_expression(lexer);
    args = args->next;
  }

  return args;
}


/*
 * <FunctionCall> -> identifier "(" <ArgList> ")"
 */
static ASTExpression* parse_function_call(Lexer* lexer)
{
  Token identifier = {0};
  ASTExpression* function_call = NULL;
  lexer_require_token(lexer, TOKEN_IDENTIFIER, &identifier);
  if (identifier.type != TOKEN_IDENTIFIER || lexer_skip_token(lexer, TOKEN_OPEN_PAREN) == false)
    return NULL;

  // No arguments
  Token look_ahead_token = lexer_look_ahead(lexer);
  if (look_ahead_token.type == TOKEN_CLOSE_PAREN)
  {
    lexer_skip_token(lexer, TOKEN_CLOSE_PAREN);
    function_call = ast_create_expression_function_call(identifier.value, NULL);
  }
  else
  {
    ASTExpression* args = parse_arg_list(lexer);
    if (args == NULL)
      return NULL;

    if (lexer_skip_token(lexer, TOKEN_CLOSE_PAREN) == false)
      return NULL;

    function_call = ast_create_expression_function_call(identifier.value, args);
  }

  return function_call;
}


/*
 * <Factor> -> ( int_literal | float_literal | string_literal | bool_literal | <lvalue> | <FunctionCall> | "(" <Expression> ")" )
 */ 
static ASTExpression* parse_factor(Lexer* lexer)
{
  Token look_ahead_token1, look_ahead_token2;
  lexer_look_ahead_2(lexer, &look_ahead_token1, &look_ahead_token2);

  if (look_ahead_token1.type == TOKEN_OPEN_PAREN)
  {
    lexer_skip_token(lexer, TOKEN_OPEN_PAREN);
    ASTExpression* expression = parse_expression(lexer);

    if (!lexer_skip_token(lexer, TOKEN_CLOSE_PAREN))
    {
      ast_destroy_expression(expression);
      return NULL;
    }
    return expression;
  }
  else if (look_ahead_token1.type == TOKEN_LITERAL_STRING)
  {
    Token literal_string;
    if (!lexer_require_token(lexer, TOKEN_LITERAL_STRING, &literal_string))
      return NULL;
    return ast_create_expression_literal_string(literal_string.value);
  }
  else if (look_ahead_token1.type == TOKEN_LITERAL_INT)
  {
    Token literal_int;
    if (!lexer_require_token(lexer, TOKEN_LITERAL_INT, &literal_int))
      return NULL;
    return ast_create_expression_literal_int(atoi(literal_int.value));

  }
  else if (look_ahead_token1.type == TOKEN_LITERAL_FLOAT)
  {
    Token literal_float;
    if (!lexer_require_token(lexer, TOKEN_LITERAL_FLOAT, &literal_float))
      return NULL;
    return ast_create_expression_literal_int(atoi(literal_float.value));
  }
  else if (look_ahead_token1.type == TOKEN_LITERAL_BOOL)
  {
    Token literal_bool;
    if (!lexer_require_token(lexer, TOKEN_LITERAL_BOOL, &literal_bool))
      return NULL;

    return ast_create_expression_literal_bool(literal_bool.value[0] == 't');
  }
  else if (look_ahead_token1.type == TOKEN_IDENTIFIER)
  {
    if (look_ahead_token2.type == TOKEN_OPEN_PAREN)
    {
      ASTExpression* function_call = parse_function_call(lexer);
      if (function_call == NULL)
        return NULL;
      return function_call;
    }

    // lvalue 
    Token identifier;
    lexer_require_token(lexer, TOKEN_IDENTIFIER, &identifier);
    return ast_create_expression_lvalue(identifier.value);
  }

  return NULL;
}


/*
 * <UnaryExpression> -> [ ( "+" | "-" )] ] <Factor>
 */ 
static ASTExpression* parse_unary_expression(Lexer* lexer)
{
  Token look_ahead_token = lexer_look_ahead(lexer);
  ASTUnaryOperator op;
  if (look_ahead_token.type == TOKEN_PLUS)
    op = OP_UNARY_PLUS;
  else if (look_ahead_token.type == TOKEN_MINUS)
    op = OP_UNARY_MINUS;
  else if (look_ahead_token.type == TOKEN_EXCLAMATION)
    op = OP_LOGICAL_NOT;
  else
  {
    return parse_factor(lexer);
  }

  lexer_skip_token(lexer, look_ahead_token.type); // skip look_ahead token
  ASTExpression* rhs = parse_factor(lexer);
  if (rhs == NULL)
    return rhs;

  return ast_create_expression_unary(op, rhs);
}


/*
 * <Term> -> <UnaryExpression> ( ( "*" | "/" | "%" ) <UnaryExpression> )*
 */ 
static ASTExpression* parse_term(Lexer* lexer)
{
  ASTExpression* term = parse_unary_expression(lexer);
  if (term == NULL)
    return NULL;

  while(true)
  {
    Token look_ahead_token = lexer_look_ahead(lexer);
    ASTTermOperator op;
    if (look_ahead_token.type == TOKEN_ASTERISK)
      op = OP_MULTIPLY;
    else if (look_ahead_token.type == TOKEN_SLASH )
      op = OP_DIVIDE;
    else if (look_ahead_token.type == TOKEN_PERCENT)
      op = OP_MOD;
    else
      break;

    lexer_get_next_token(lexer); // consume the look_ahead token
    ASTExpression* rhs = parse_unary_expression(lexer);
    if (rhs == NULL)
      return NULL;

    term = ast_create_expression_term(term, op, rhs);
  }  

  return term;
}


/*
 * <NumExpression> -> <Term> ( ( "+" | "-" ) <Term> )*
 */ 
static ASTExpression* parse_num_expression(Lexer* lexer)
{
  ASTExpression* num_expression = parse_term(lexer);
  if (num_expression == NULL)
    return NULL;

  while(true)
  {
    Token look_ahead_token = lexer_look_ahead(lexer);
    ASTFactorOperator op;
    if (look_ahead_token.type == TOKEN_PLUS)
      op = OP_ADD;
    else if (look_ahead_token.type == TOKEN_MINUS)
      op = OP_SUBTRACT;
    else
      break;

    lexer_get_next_token(lexer); // consume the look_ahead token
    ASTExpression* rhs = parse_term(lexer);
    if (rhs == NULL)
      return NULL;

    num_expression = ast_create_expression_factor(num_expression, op, rhs);
  } 

  return num_expression;
}


/*
 * <AssignmentStatement> -> <lvalue> "=" <Expression>
 */
static ASTStatement* parse_assignment_statement(Lexer* lexer)
{
  Token identifier;
  if (!lexer_require_token(lexer, TOKEN_IDENTIFIER, &identifier))
    return NULL;

  if (!lexer_skip_token(lexer, TOKEN_OP_ASSIGN))
    return NULL;

  ASTExpression* rhs = parse_expression(lexer);
  if (rhs == NULL)
    return NULL;
  return ast_create_statement_assignment(identifier.value, rhs);
}


/*
 * <ifStatement> -> "if" "(" <Expression> ")" <Statement> [ "else" <Statement> ]
 */
static ASTStatement* parse_if_statement(Lexer* lexer)
{
  // if ( Condition )
  if (lexer_skip_token(lexer, TOKEN_IF) == false
      || lexer_skip_token(lexer, TOKEN_OPEN_PAREN) == false)
    return NULL;
  ASTExpression* condition = parse_expression(lexer);
  if (condition == NULL)
    return NULL;

  if (lexer_skip_token(lexer, TOKEN_CLOSE_PAREN) == false)
    return NULL;

  // then_block
  ASTStatement* then_block = parse_statement(lexer);
  if (then_block == NULL)
    return NULL;

  Token look_ahead_token = lexer_look_ahead(lexer);
  ASTStatement* else_block = NULL;
  if (look_ahead_token.type == TOKEN_ELSE)
  {
    lexer_skip_token(lexer, TOKEN_ELSE);
    else_block = parse_statement(lexer);
    // else_block
    if (else_block == NULL)
      return NULL;
  }
  return ast_create_statement_if(condition, then_block, else_block);
}


/*
 * <WhileStatement> -> "while" "(" <Expression> ")" <Statement> 
 */
static ASTStatement* parse_while_statement(Lexer* lexer)
{
  // while ( Condition )
  if (lexer_skip_token(lexer, TOKEN_WHILE) == false
      || lexer_skip_token(lexer, TOKEN_OPEN_PAREN) == false)
    return NULL;
  ASTExpression* condition = parse_expression(lexer);
  if (condition == NULL)
    return NULL;

  if (lexer_skip_token(lexer, TOKEN_CLOSE_PAREN) == false)
  {
    ast_destroy_expression(condition);
    return NULL;
  }

  // then_block
  ASTStatement* then_block = parse_statement(lexer);
  if (then_block == NULL)
  {
    ast_destroy_expression(condition);
    return NULL;
  }

  return ast_create_statement_while(condition, then_block);
}


/*
 * <ForStatement> -> "for" "(" [<AssignmentStatement>] ";" [<expression>] ";" [<AssignmentStatement>] ")" <Statement> 
 */
static ASTStatement* parse_for_statement(Lexer* lexer)
{
  // while ( Condition )
  if (lexer_skip_token(lexer, TOKEN_FOR) == false
      || lexer_skip_token(lexer, TOKEN_OPEN_PAREN) == false)
    return NULL;

  ASTStatement* init = parse_assignment_statement(lexer);
  if (lexer_skip_token(lexer, TOKEN_SEMICOLON) == false)
  {
    ast_destroy_statement(init);
    return NULL;
  }

  ASTExpression* condition = parse_expression(lexer);
  if (lexer_skip_token(lexer, TOKEN_SEMICOLON) == false)
  {
    ast_destroy_statement(init);
    ast_destroy_expression(condition);
    return NULL;
  }

  ASTStatement* update = parse_assignment_statement(lexer);
  if (lexer_skip_token(lexer, TOKEN_CLOSE_PAREN) == false)
  {
    ast_destroy_statement(init);
    ast_destroy_expression(condition);
    ast_destroy_statement(update);
    return NULL;
  }

  // then_block
  ASTStatement* then_block = parse_statement(lexer);
  if (then_block == NULL)
  {
    ast_destroy_statement(init);
    ast_destroy_expression(condition);
    ast_destroy_statement(update);
    return NULL;
  }

  return ast_create_statement_for(init, condition, update, then_block);
}


/*
 * <ReturnStatement> -> "return" [ <Expression> ]
 */
static ASTStatement* parse_return_statement(Lexer* lexer)
{
  ASTExpression* expression = parse_expression(lexer);
  if (expression == NULL)
    return NULL;
  return ast_create_statement_return(parse_expression(lexer));
};


/*
 * <FunctionBody> -> "(" <ParamList> ")" <Statement>
 */
static bool parse_function_body(Lexer* lexer)
{
  UNUSED(lexer);
  return false;
}


/*
 * <FunctionDeclStatement> -> "function" identifier "(" [ identifier ( "," identifier )* ] ")" <FunctionBody>
 */
static bool parse_function_declaration_statement(Lexer* lexer)
{
  UNUSED(lexer);
  return false;
}


/*
 * <Expression> -> <NumExpression> [ ( "<" | ">" | "<=" | ">=" | "==" | "!=" ) <NumExpression> ]
 */ 
static ASTExpression* parse_expression(Lexer* lexer)
{
  ASTExpression* expression = parse_num_expression(lexer);
  if (expression == NULL)
    return NULL;

  while(true)
  {
    Token look_ahead_token = lexer_look_ahead(lexer);
    ASTComparisonOperator op;
    if (look_ahead_token.type == TOKEN_OP_LT)
      op = OP_LT;
    else if (look_ahead_token.type == TOKEN_OP_GT)
      op = OP_GT;
    else if (look_ahead_token.type == TOKEN_OP_LTE)
      op = OP_LTE;
    else if (look_ahead_token.type == TOKEN_OP_GTE)
      op = OP_GTE;
    else if (look_ahead_token.type == TOKEN_OP_EQ)
      op = OP_EQ;
    else if (look_ahead_token.type == TOKEN_OP_NEQ)
      op = OP_NEQ;
    else
      break;

    if (lexer_skip_token(lexer, look_ahead_token.type) == false)
      return NULL;

    ASTExpression* rhs = parse_num_expression(lexer);
    if (rhs == NULL)
      break;

    expression = ast_create_expression_comparison(expression, op, rhs);
  }

  return expression;
};


/*
 * <Statement> -> ( <FunctionCall> | <InputStatement> | <ReturnStatement> | <AssignmentStatement> <FunctionDeclStatement> | <IfStatement> | <ForStatement> | <WhileStatement> | "{" <StatementList> "}")
 */
static ASTStatement* parse_statement(Lexer *lexer)
{
  Token look_ahead_token1, look_ahead_token2;
  lexer_look_ahead_2(lexer, &look_ahead_token1, &look_ahead_token2);

  switch (look_ahead_token1.type)
  {
    case TOKEN_OPEN_BRACE:
      {
        if (lexer_skip_token(lexer, TOKEN_OPEN_BRACE) == false)
          return NULL;

        ASTStatement* block = parse_statement_list(lexer);
        if (block == NULL)
          return NULL;

        if (lexer_skip_token(lexer, TOKEN_CLOSE_BRACE) == false)
          return NULL;

        return block;
      }

    case TOKEN_IDENTIFIER:
      {
        if (look_ahead_token2.type == TOKEN_OPEN_PAREN)
        {
          // FUNCTION CALL
          ASTExpression* function_call = parse_function_call(lexer);
          if(function_call)
          {
            return ast_create_statement_function_call(function_call);
          }
        }
        else if (look_ahead_token2.type == TOKEN_OP_ASSIGN)
        {
          // ASSIGNMENT
          ASTStatement* statement = parse_assignment_statement(lexer);
          if(statement)
            return statement;
        }
        break;
      }

    case TOKEN_RETURN:
      {
        ASTStatement* statement = parse_return_statement(lexer);
        if(statement)
          return statement;
        break;
      }

    case TOKEN_FOR:
      {
        return parse_for_statement(lexer);
        break;
      }

    case TOKEN_WHILE:
      {
        return parse_while_statement(lexer);
        break;
      }

    case TOKEN_IF:
      return parse_if_statement(lexer);

    default:
      if (look_ahead_token1.type == TOKEN_EOF)
      {
        lexer_skip_token(lexer, TOKEN_EOF); // consume the look_ahead token
        return NULL;
      }
  }

  return NULL;
}


/*
 * <StatementList> -> <Statement>; [ <StatementList> ]
 */
static ASTStatement* parse_statement_list(Lexer* lexer)
{
  ASTStatement* first_statement = parse_statement(lexer);
  ASTStatement* statement = first_statement;

  while(statement)
  {
    Token look_ahead_token = lexer_look_ahead(lexer);
    if (look_ahead_token.type != TOKEN_SEMICOLON)
      break;

    lexer_skip_token(lexer, TOKEN_SEMICOLON);
    statement->next = parse_statement_list(lexer);
    statement = statement->next;
  }

  return first_statement;
};


//
// Public functions
//


void lexer_init(Lexer *lexer, const char *buffer)
{
  lexer->buffer = (char *)buffer;
  lexer->position = 0;
  lexer->current_char = buffer[lexer->position]; // Start with the first character
  lexer->line = 1;
  lexer->column = 1;
}

/*
 * <Program> -> ( <StatementList> )*
 */
ASTProgram* parse_program(Lexer *lexer)
{
  ASTStatement* body = parse_statement_list(lexer);
  return ast_create_program(body);
}

