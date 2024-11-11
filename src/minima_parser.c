/**
 * @file parser.c
 * @brief Defines the lexer and parser for tokenizing and parsing source code.
 *
 * This header contains structures and functions for the lexer and parser, 
 * including token definitions, lexer operations, and parsing functions.
 * 
 * @author marciofmv
 */

//TODO: Arrays, Function declaration, Logical AND and OR operators

#include "common.h"
#include "minima_ast.h"
#include "minima_parser.h"

typedef enum TokenType_e
{
  TOKEN_ERROR,              // Represents a tokenizer error
  TOKEN_OPEN_CODE_BLOCK,    // <?
  TOKEN_CLOSE_CODE_BLOCK,   // ?>
  TOKEN_LOGICAL_AND,        // &&
  TOKEN_LOGICAL_OR,         // ||
  TOKEN_OP_ASSIGN,          // =
  TOKEN_OP_EQ,              // ==
  TOKEN_OP_NEQ,             // !=
  TOKEN_OP_LT,              // <
  TOKEN_OP_LTE,             // <=
  TOKEN_OP_GT,              // >
  TOKEN_OP_GTE,             // >=
  TOKEN_OPEN_PAREN,         // (
  TOKEN_CLOSE_PAREN,        // )
  TOKEN_OPEN_BRACE,         // {
  TOKEN_CLOSE_BRACE,        // }
  TOKEN_OPEN_BACKET,        // [
  TOKEN_CLOSE_BRACKET,      // ]
  TOKEN_ASTERISK,           // *
  TOKEN_SLASH,              // /
  TOKEN_PERCENT,            // %
  TOKEN_COMMA,              // ,
  TOKEN_DOT,                // .
  TOKEN_EXCLAMATION,        // !
  TOKEN_PLUS,               // +
  TOKEN_MINUS,              // -
  TOKEN_SEMICOLON,          // ;
  TOKEN_IF,                 // if keyword
  TOKEN_ELSE,               // else keyword
  TOKEN_FOR,                // for keyword
  TOKEN_WHILE,              // while keyword
  TOKEN_RETURN,             // return keyword
  TOKEN_INCLUDE,            // include keyword
  TOKEN_IDENTIFIER,         // <variable names>
  TOKEN_LITERAL_INT,        // [0-9]+
  TOKEN_LITERAL_FLOAT,      // [0-9]*"."[0-9]+
  TOKEN_LITERAL_STRING,     // "double quote string!"
  TOKEN_LITERAL_BOOL,       // true or false literal
  TOKEN_EOF,                // End of file/stream
  TOKEN_COUNT_
} TokenType;

#define MI_PARSER_MAX_TOKEN_LENGTH 100
typedef struct Token_t
{
  TokenType type;
  char value[MI_PARSER_MAX_TOKEN_LENGTH];
} Token;


typedef struct Lexer_t
{
  char    *buffer;      // Buffer containing the file contents
  char    current_char; // Current character
  char    next_char;    // Next character
  size_t  position;     // Current position in the buffer
  int     line;         // Current line number
  int     column;       // Current column number
  bool    raw_mode;
} Lexer;

#define report_error(lexer, message) log_error("Syntax error at line %d, column %d: %s\n",\
    lexer->line, lexer->column, message);


#define report_error_unexpected_token(lexer, token_type) log_error("Sytax error at %d, %d: Unexpected '%s' token \n",\
    lexer->line, lexer->column, token_get_name(token_type))

const char* token_get_name(TokenType token)
{
  static const char *tokenNames[TOKEN_COUNT_] = {
    [TOKEN_ERROR]           = "Invalid",
    [TOKEN_OPEN_CODE_BLOCK] = "Code block open",
    [TOKEN_CLOSE_CODE_BLOCK]= "Code block close",
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


#define s_lexer_get_next_token(lexer) s_lexer_get_next_token_(lexer, false)
static Token s_lexer_get_next_token_(Lexer *lexer, bool suppress_errors);
static ASTExpression* s_parse_term(Lexer* lexer);
static ASTExpression* s_parse_expression(Lexer* lexer);
static ASTStatement* s_parse_statement(Lexer *lexer);
static ASTStatement* s_parse_statement_list(Lexer* lexer);
static ASTExpression* s_parse_logical_expression(Lexer* lexer);
static ASTExpression* s_parse_logical_expression_and(Lexer* lexer);


static void s_lexer_advance(Lexer *lexer)
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


static Token s_lexer_look_ahead(Lexer* lexer)
{
  Lexer chekpoint = *lexer;
  Token t = s_lexer_get_next_token_(lexer, true);
  *lexer = chekpoint;
  return t;
}


static void s_lexer_look_ahead_2(Lexer* lexer, Token* token1, Token* token2)
{
  Lexer chekpoint = *lexer;
  *token1 = s_lexer_get_next_token_(lexer, true);

  // we don't want to start parsing stuff not meant to be parsed.
  // if somethig is put past a close block token, consider it is in raw mode
  if (token1->type == TOKEN_CLOSE_CODE_BLOCK)
    lexer->raw_mode = true;

  *token2 = s_lexer_get_next_token_(lexer, true);
  *lexer = chekpoint;
}


static void s_lexer_skip_until_next_line(Lexer *lexer)
{
  while (lexer->current_char != '\n' && lexer->current_char != '\0')
  {
    s_lexer_advance(lexer);
  }
}


static void s_lexer_skip_whitespace(Lexer *lexer)
{
  while (isspace(lexer->current_char) || lexer->current_char == '#')
  {
    if (lexer->current_char == '#')
    {
      s_lexer_skip_until_next_line(lexer);  // Skip comment line
    } else
    {
      s_lexer_advance(lexer);  // Skip whitespace
    }
  }
}


static Token s_lexer_get_identifier(Lexer *lexer)
{

  Token token;
  int i = 0;

  while (isalpha(lexer->current_char) || lexer->current_char == '_' ||  lexer->current_char == '.')
  {
    token.value[i++] = lexer->current_char;
    s_lexer_advance(lexer);
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
  else token.type = TOKEN_IDENTIFIER;

  return token;
}


//TODO: Fix this! Very long string literals will cause a crash here.
static Token s_lexer_get_literal_string(Lexer *lexer)
{
  Token out;

  // Expecting a starting quote
  if (lexer->current_char == '"')
  {
    int i = 0;
    bool handle_escape_char = false;
    s_lexer_advance(lexer);  // Move past the opening quote
    while (lexer->current_char != '"' && lexer->current_char != '\0')
    {
      char c = lexer->current_char;

      // handle scape characters
      if (handle_escape_char)
      {
        if (c == 'n')       { c = '\n'; i--; }
        else if (c == 't')  { c = '\t'; i--; }
        else if (c == '\\') { c = '\\'; i--; }
        else if (c == 'r')  { c = '\r'; i--; }
        else log_warning("Warning at line %d, column %d: Unknown escape character '%c'\n", lexer->line, lexer->column, c);

        handle_escape_char = false;
      }
      else
      {
        handle_escape_char = c == '\\';
      }

      // output character
      out.value[i++] = c;
      s_lexer_advance(lexer);
    }
    out.value[i] = '\0';  // Null terminate the string
    if (lexer->current_char == '"')
    {
      s_lexer_advance(lexer);  // Move past the closing quote
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


static bool s_lexer_skip_token(Lexer* lexer, TokenType expected_type)
{
  Token t = s_lexer_get_next_token(lexer);
  bool result = (t.type == expected_type);
  if (!result)
     log_error("Sytax error at %d, %d: Expecting '%s' but '%s' found. \n",
         lexer->line, lexer->column, token_get_name(expected_type), token_get_name(t.type));
  return true;
}


static Token s_lexer_get_next_token_(Lexer *lexer, bool suppress_errors)
{
  Token token;
  s_lexer_skip_whitespace(lexer);

  if (lexer->current_char == '"')
  {
    return s_lexer_get_literal_string(lexer);
  }

  if (isalpha(lexer->current_char))
  {
    return s_lexer_get_identifier(lexer);
  } 
  else if (isdigit(lexer->current_char) || lexer->current_char == '.')
  {
    int dot_count = 0;
    if (lexer->current_char == '.')
      dot_count++;

    int i = 0;
    while (isdigit(lexer->current_char) || (dot_count <= 1 && lexer->current_char == '.'))
    {
      if (lexer->current_char == '.')
        dot_count++;

      token.value[i++] = lexer->current_char;
      s_lexer_advance(lexer);
    }
    token.value[i] = '\0';

    if (dot_count == 0)
      token.type = TOKEN_LITERAL_INT;
    else
      token.type = TOKEN_LITERAL_FLOAT;
    return token;
  }
  else if (lexer->current_char == '<' && lexer->next_char == '?')
  {
    token.type = TOKEN_OPEN_CODE_BLOCK;
    token.value[0] = lexer->current_char;
    token.value[1] = lexer->next_char;
    token.value[2] = '\0';
    s_lexer_advance(lexer);
    s_lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '?' && lexer->next_char == '>')
  {
    token.type = TOKEN_CLOSE_CODE_BLOCK;
    token.value[0] = lexer->current_char;
    token.value[1] = lexer->next_char;
    token.value[2] = '\0';
    s_lexer_advance(lexer);
    s_lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '&' && lexer->next_char == '&')
  {
    token.type = TOKEN_LOGICAL_AND;
    token.value[0] = lexer->current_char;
    token.value[1] = lexer->next_char;
    token.value[2] = '\0';
    s_lexer_advance(lexer);
    s_lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '|' && lexer->next_char == '|')
  {
    token.type = TOKEN_LOGICAL_OR;
    token.value[0] = lexer->current_char;
    token.value[1] = lexer->next_char;
    token.value[2] = '\0';
    s_lexer_advance(lexer);
    s_lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '>' && lexer->next_char == '=')
  {
    token.type = TOKEN_OP_GTE;
    token.value[0] = lexer->current_char;
    token.value[1] = lexer->next_char;
    token.value[2] = '\0';
    s_lexer_advance(lexer);
    s_lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '<' && lexer->next_char == '=')
  {
    token.type = TOKEN_OP_LTE;
    token.value[0] = lexer->current_char;
    token.value[1] = lexer->next_char;
    token.value[2] = '\0';
    s_lexer_advance(lexer);
    s_lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '=' && lexer->next_char == '=')
  {
    token.type = TOKEN_OP_EQ;
    token.value[0] = lexer->current_char;
    token.value[1] = lexer->next_char;
    token.value[2] = '\0';
    s_lexer_advance(lexer);
    s_lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '!' && lexer->next_char == '=')
  {
    token.type = TOKEN_OP_NEQ;
    token.value[0] = lexer->current_char;
    token.value[1] = lexer->next_char;
    token.value[2] = '\0';
    s_lexer_advance(lexer);
    s_lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '>')
  {
    token.type = TOKEN_OP_GT;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    s_lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '<')
  {
    token.type = TOKEN_OP_LT;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    s_lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '=')
  {
    token.type = TOKEN_OP_ASSIGN;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    s_lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '(')
  {
    token.type = TOKEN_OPEN_PAREN;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    s_lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == ')')
  {
    token.type = TOKEN_CLOSE_PAREN;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    s_lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '+')
  {
    token.type = TOKEN_PLUS;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    s_lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '-')
  {
    token.type = TOKEN_MINUS;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    s_lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '*')
  {
    token.type = TOKEN_ASTERISK;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    s_lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '/')
  {
    token.type = TOKEN_SLASH;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    s_lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '%')
  {
    token.type = TOKEN_PERCENT;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    s_lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == ',')
  {
    token.type = TOKEN_COMMA;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    s_lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == ';')
  {
    token.type = TOKEN_SEMICOLON;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    s_lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '.')
  {
    token.type = TOKEN_DOT;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    s_lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '!')
  {
    token.type = TOKEN_EXCLAMATION;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    s_lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '{')
  {
    token.type = TOKEN_OPEN_BRACE;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    s_lexer_advance(lexer);
    return token;
  }
  else if (lexer->current_char == '}')
  {
    token.type = TOKEN_CLOSE_BRACE;
    token.value[0] = lexer->current_char;
    token.value[1] = '\0';
    s_lexer_advance(lexer);
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


static bool s_lexer_require_token(Lexer* lexer, TokenType expected_type, Token* out)
{
  *out = s_lexer_get_next_token(lexer);
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
static ASTExpression* s_parse_arg_list(Lexer* lexer)
{
  //ASTExpression* arg_list = s_parse_expression(lexer);
  ASTExpression* arg_list = s_parse_logical_expression(lexer);
  ASTExpression* args = arg_list;

  while (args != NULL)
  {
    Token look_ahead_token = s_lexer_look_ahead(lexer);
    if (look_ahead_token.type != TOKEN_COMMA)
      break;

    s_lexer_skip_token(lexer, TOKEN_COMMA);
    //args->next = s_parse_expression(lexer);
    args->next = s_parse_logical_expression(lexer);
    args = args->next;
  }

  return args;
}


/*
 * <FunctionCall> -> identifier "(" <ArgList> ")"
 */
static ASTExpression* s_parse_function_call(Lexer* lexer)
{
  Token identifier = {0};
  ASTExpression* function_call = NULL;
  s_lexer_require_token(lexer, TOKEN_IDENTIFIER, &identifier);
  if (identifier.type != TOKEN_IDENTIFIER || s_lexer_skip_token(lexer, TOKEN_OPEN_PAREN) == false)
    return NULL;

  // No arguments
  Token look_ahead_token = s_lexer_look_ahead(lexer);
  if (look_ahead_token.type == TOKEN_CLOSE_PAREN)
  {
    s_lexer_skip_token(lexer, TOKEN_CLOSE_PAREN);
    function_call = mi_ast_expression_create_function_call(identifier.value, NULL);
  }
  else
  {
    ASTExpression* args = s_parse_arg_list(lexer);
    if (args == NULL)
      return NULL;

    if (s_lexer_skip_token(lexer, TOKEN_CLOSE_PAREN) == false)
      return NULL;

    function_call = mi_ast_expression_create_function_call(identifier.value, args);
  }

  return function_call;
}


/*
 * <Factor> -> ( int_literal | float_literal | string_literal | bool_literal | <lvalue> | <FunctionCall> | "(" <LogicalExpression> ")" )
 */ 
static ASTExpression* s_parse_factor(Lexer* lexer)
{
  Token look_ahead_token1, look_ahead_token2;
  s_lexer_look_ahead_2(lexer, &look_ahead_token1, &look_ahead_token2);

  if (look_ahead_token1.type == TOKEN_OPEN_PAREN)
  {
    s_lexer_skip_token(lexer, TOKEN_OPEN_PAREN);
    //ASTExpression* expression = s_parse_expression(lexer);
    ASTExpression* expression = s_parse_logical_expression(lexer);

    if (!s_lexer_skip_token(lexer, TOKEN_CLOSE_PAREN))
    {
      mi_ast_expression_destroy(expression);
      return NULL;
    }
    return expression;
  }
  else if (look_ahead_token1.type == TOKEN_LITERAL_STRING)
  {
    Token literal_string;
    if (!s_lexer_require_token(lexer, TOKEN_LITERAL_STRING, &literal_string))
      return NULL;
    return mi_ast_expression_create_literal_string(literal_string.value);
  }
  else if (look_ahead_token1.type == TOKEN_LITERAL_INT)
  {
    Token literal_int;
    if (!s_lexer_require_token(lexer, TOKEN_LITERAL_INT, &literal_int))
      return NULL;
    return mi_ast_expression_create_literal_int(atoi(literal_int.value));

  }
  else if (look_ahead_token1.type == TOKEN_LITERAL_FLOAT)
  {
    Token literal_float;
    if (!s_lexer_require_token(lexer, TOKEN_LITERAL_FLOAT, &literal_float))
      return NULL;
    return mi_ast_expression_create_literal_float(atof(literal_float.value));
  }
  else if (look_ahead_token1.type == TOKEN_LITERAL_BOOL)
  {
    Token literal_bool;
    if (!s_lexer_require_token(lexer, TOKEN_LITERAL_BOOL, &literal_bool))
      return NULL;

    return mi_ast_expression_create_literal_bool(literal_bool.value[0] == 't');
  }
  else if (look_ahead_token1.type == TOKEN_IDENTIFIER)
  {
    if (look_ahead_token2.type == TOKEN_OPEN_PAREN)
    {
      ASTExpression* function_call = s_parse_function_call(lexer);
      if (function_call == NULL)
        return NULL;
      return function_call;
    }

    // lvalue 
    Token identifier;
    s_lexer_require_token(lexer, TOKEN_IDENTIFIER, &identifier);
    return mi_ast_expression_create_lvalue(identifier.value);
  }

  return NULL;
}


/*
 * <UnaryExpression> -> [ ( "+" | "-" )] ] <Factor>
 */ 
static ASTExpression* s_parse_unary_expression(Lexer* lexer)
{
  Token look_ahead_token = s_lexer_look_ahead(lexer);
  ASTUnaryOperator op;
  if (look_ahead_token.type == TOKEN_PLUS)
    op = OP_UNARY_PLUS;
  else if (look_ahead_token.type == TOKEN_MINUS)
    op = OP_UNARY_MINUS;
  else if (look_ahead_token.type == TOKEN_EXCLAMATION)
    op = OP_LOGICAL_NOT;
  else
  {
    return s_parse_factor(lexer);
  }

  s_lexer_skip_token(lexer, look_ahead_token.type); // skip look_ahead token
  ASTExpression* rhs = s_parse_factor(lexer);
  if (rhs == NULL)
    return rhs;

  return mi_ast_expression_create_unary(op, rhs);
}


/*
 * <Term> -> <UnaryExpression> ( ( "*" | "/" | "%" ) <UnaryExpression> )*
 */ 
static ASTExpression* s_parse_term(Lexer* lexer)
{
  ASTExpression* term = s_parse_unary_expression(lexer);
  if (term == NULL)
    return NULL;

  while(true)
  {
    Token look_ahead_token = s_lexer_look_ahead(lexer);
    ASTTermOperator op;
    if (look_ahead_token.type == TOKEN_ASTERISK)
      op = OP_MULTIPLY;
    else if (look_ahead_token.type == TOKEN_SLASH )
      op = OP_DIVIDE;
    else if (look_ahead_token.type == TOKEN_PERCENT)
      op = OP_MOD;
    else
      break;

    s_lexer_get_next_token(lexer); // consume the look_ahead token
    ASTExpression* rhs = s_parse_unary_expression(lexer);
    if (rhs == NULL)
      return NULL;

    term = mi_ast_expression_create_term(term, op, rhs);
  }  

  return term;
}


/*
 * <NumExpression> -> <Term> ( ( "+" | "-" ) <Term> )*
 */ 
static ASTExpression* s_parse_num_expression(Lexer* lexer)
{
  ASTExpression* num_expression = s_parse_term(lexer);
  if (num_expression == NULL)
    return NULL;

  while(true)
  {
    Token look_ahead_token = s_lexer_look_ahead(lexer);
    ASTFactorOperator op;
    if (look_ahead_token.type == TOKEN_PLUS)
      op = OP_ADD;
    else if (look_ahead_token.type == TOKEN_MINUS)
      op = OP_SUBTRACT;
    else
      break;

    s_lexer_get_next_token(lexer); // consume the look_ahead token
    ASTExpression* rhs = s_parse_term(lexer);
    if (rhs == NULL)
      return NULL;

    num_expression = mi_ast_expression_create_factor(num_expression, op, rhs);
  } 

  return num_expression;
}


/*
 * <AssignmentStatement> -> <lvalue> "=" <Expression>
 */
static ASTStatement* s_parse_assignment_statement(Lexer* lexer)
{
  Token identifier;
  if (!s_lexer_require_token(lexer, TOKEN_IDENTIFIER, &identifier))
    return NULL;

  if (!s_lexer_skip_token(lexer, TOKEN_OP_ASSIGN))
    return NULL;

  ASTExpression* rhs = s_parse_logical_expression(lexer);
  if (rhs == NULL)
    return NULL;
  return mi_ast_statement_create_assignment(identifier.value, rhs);
}


/*
 * <ifStatement> -> "if" "(" <Expression> ")" <Statement> [ "else" <Statement> ]
 */
static ASTStatement* s_parse_if_statement(Lexer* lexer)
{
  // if ( Condition )
  if (s_lexer_skip_token(lexer, TOKEN_IF) == false
      || s_lexer_skip_token(lexer, TOKEN_OPEN_PAREN) == false)
    return NULL;
  ASTExpression* condition = s_parse_logical_expression(lexer);
  if (condition == NULL)
    return NULL;

  if (s_lexer_skip_token(lexer, TOKEN_CLOSE_PAREN) == false)
    return NULL;

  // then_block
  ASTStatement* then_block = s_parse_statement(lexer);
  if (then_block == NULL)
    return NULL;

  Token look_ahead_token = s_lexer_look_ahead(lexer);
  ASTStatement* else_block = NULL;
  if (look_ahead_token.type == TOKEN_ELSE)
  {
    s_lexer_skip_token(lexer, TOKEN_ELSE);
    else_block = s_parse_statement(lexer);
    // else_block
    if (else_block == NULL)
      return NULL;
  }
  return mi_ast_statement_create_if(condition, then_block, else_block);
}


/*
 * <WhileStatement> -> "while" "(" <Expression> ")" <Statement> 
 */
static ASTStatement* s_parse_while_statement(Lexer* lexer)
{
  // while ( Condition )
  if (s_lexer_skip_token(lexer, TOKEN_WHILE) == false
      || s_lexer_skip_token(lexer, TOKEN_OPEN_PAREN) == false)
    return NULL;
  ASTExpression* condition = s_parse_logical_expression(lexer);
  if (condition == NULL)
    return NULL;

  if (s_lexer_skip_token(lexer, TOKEN_CLOSE_PAREN) == false)
  {
    mi_ast_expression_destroy(condition);
    return NULL;
  }

  // then_block
  ASTStatement* then_block = s_parse_statement(lexer);
  if (then_block == NULL)
  {
    mi_ast_expression_destroy(condition);
    return NULL;
  }

  return mi_ast_statement_create_while(condition, then_block);
}


/*
 * <ForStatement> -> "for" "(" [<AssignmentStatement>] ";" [<expression>] ";" [<AssignmentStatement>] ")" <Statement> 
 */
static ASTStatement* s_parse_for_statement(Lexer* lexer)
{
  // while ( Condition )
  if (s_lexer_skip_token(lexer, TOKEN_FOR) == false
      || s_lexer_skip_token(lexer, TOKEN_OPEN_PAREN) == false)
    return NULL;

  ASTStatement* init = s_parse_assignment_statement(lexer);
  if (s_lexer_skip_token(lexer, TOKEN_SEMICOLON) == false)
  {
    mi_ast_statement_destroy(init);
    return NULL;
  }

  ASTExpression* condition = s_parse_logical_expression(lexer);
  if (s_lexer_skip_token(lexer, TOKEN_SEMICOLON) == false)
  {
    mi_ast_statement_destroy(init);
    mi_ast_expression_destroy(condition);
    return NULL;
  }

  ASTStatement* update = s_parse_assignment_statement(lexer);
  if (s_lexer_skip_token(lexer, TOKEN_CLOSE_PAREN) == false)
  {
    mi_ast_statement_destroy(init);
    mi_ast_expression_destroy(condition);
    mi_ast_statement_destroy(update);
    return NULL;
  }

  // then_block
  ASTStatement* then_block = s_parse_statement(lexer);
  if (then_block == NULL)
  {
    mi_ast_statement_destroy(init);
    mi_ast_expression_destroy(condition);
    mi_ast_statement_destroy(update);
    return NULL;
  }

  return mi_ast_statement_create_for(init, condition, update, then_block);
}


/*
 * <ReturnStatement> -> "return" [ <Expression> ]
 */
static ASTStatement* s_parse_return_statement(Lexer* lexer)
{
  ASTExpression* expression = s_parse_logical_expression(lexer);
  if (expression == NULL)
    return NULL;
  return mi_ast_statement_create_return(s_parse_logical_expression(lexer));
};


/*
 * <FunctionBody> -> "(" <ParamList> ")" <Statement>
 */
static bool s_parse_function_body(Lexer* lexer)
{
  UNUSED(lexer);
  return false;
}


/*
 * <FunctionDeclStatement> -> "function" identifier "(" [ identifier ( "," identifier )* ] ")" <FunctionBody>
 */
static bool s_parse_function_declaration_statement(Lexer* lexer)
{
  UNUSED(lexer);
  return false;
}


static ASTExpression* s_parse_logical_expression_and(Lexer* lexer)
{
  ASTExpression* expression = s_parse_expression(lexer);
  if (expression == NULL)
    return NULL;

  while(true)
  {
    Token look_ahead_token = s_lexer_look_ahead(lexer);
    ASTLogicalOperator op;
    if (look_ahead_token.type == TOKEN_LOGICAL_AND)
      op = OP_LOGICAL_AND;
    else
      break;

    if (s_lexer_skip_token(lexer, look_ahead_token.type) == false)
      return NULL;

    ASTExpression* rhs = s_parse_expression(lexer);
    if (rhs == NULL)
      break;

    expression = mi_ast_expression_create_logical(expression, op, rhs);
  }

  return expression;
}

static ASTExpression* s_parse_logical_expression(Lexer* lexer)
{
  ASTExpression* expression = s_parse_logical_expression_and(lexer);
  if (expression == NULL)
    return NULL;

  while(true)
  {
    Token look_ahead_token = s_lexer_look_ahead(lexer);
    ASTLogicalOperator op;
    if (look_ahead_token.type == TOKEN_LOGICAL_OR)
      op = OP_LOGICAL_OR;
    else
      break;

    if (s_lexer_skip_token(lexer, look_ahead_token.type) == false)
      return NULL;

    ASTExpression* rhs = s_parse_logical_expression_and(lexer);
    if (rhs == NULL)
      break;

    expression = mi_ast_expression_create_logical(expression, op, rhs);
  }

  return expression;
}


/*
 * <Expression> -> <NumExpression> [ ( "<" | ">" | "<=" | ">=" | "==" | "!=" ) <NumExpression> ]
 */ 
static ASTExpression* s_parse_expression(Lexer* lexer)
{
  ASTExpression* expression = s_parse_num_expression(lexer);
  if (expression == NULL)
    return NULL;

  while(true)
  {
    Token look_ahead_token = s_lexer_look_ahead(lexer);
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

    if (s_lexer_skip_token(lexer, look_ahead_token.type) == false)
      return NULL;

    ASTExpression* rhs = s_parse_num_expression(lexer);
    if (rhs == NULL)
      break;

    expression = mi_ast_expression_create_comparison(expression, op, rhs);
  }

  return expression;
};

/**
 * A raw expression is anything outside of <? ?> block
 */
ASTStatement* s_parse_raw(Lexer* lexer)
{
  char* start = &lexer->buffer[lexer->position];
  size_t len = 0;
  while(true)
  {
    if (lexer->raw_mode == false  || lexer->current_char == 0)
      break;

    // Ignore new lines immediately after closing code blocks
    if (*start == '\r' || *start == '\n' || *start == '\t')
    {
      start++;
      s_lexer_advance(lexer);
      continue;
    }

    // A raw block ends at EOF or <%
    if (lexer->current_char == '<' && lexer->next_char == '?')
    {
      break;
    }
    len++;
    s_lexer_advance(lexer);
  }

  if (len > 0)
  {
    return mi_ast_statement_create_raw(start, len);
  }

  return s_parse_statement(lexer);
}

/*
 * <Statement> -> ( <FunctionCall> | <InputStatement> | <ReturnStatement> | <AssignmentStatement> <FunctionDeclStatement> | <IfStatement> | <ForStatement> | <WhileStatement> | "{" <StatementList> "}")
 */
static ASTStatement* s_parse_statement(Lexer *lexer)
{
  Token look_ahead_token1, look_ahead_token2;
  s_lexer_look_ahead_2(lexer, &look_ahead_token1, &look_ahead_token2);

  switch (look_ahead_token1.type)
  {
    case TOKEN_OPEN_CODE_BLOCK:
      {
        lexer->raw_mode = false;
        s_lexer_skip_token(lexer, TOKEN_OPEN_CODE_BLOCK);
        return s_parse_raw(lexer);
      }
    case TOKEN_CLOSE_CODE_BLOCK:
      {
        lexer->raw_mode = true;
        s_lexer_skip_token(lexer, TOKEN_CLOSE_CODE_BLOCK);
        return s_parse_raw(lexer);
      }
    case TOKEN_OPEN_BRACE:
      {
        if (s_lexer_skip_token(lexer, TOKEN_OPEN_BRACE) == false)
          return NULL;

        ASTStatement* block = s_parse_statement_list(lexer);
        if (block == NULL)
          return NULL;

        if (s_lexer_skip_token(lexer, TOKEN_CLOSE_BRACE) == false)
          return NULL;

        return block;
      }

    case TOKEN_IDENTIFIER:
      {
        if (look_ahead_token2.type == TOKEN_OPEN_PAREN)
        {
          // FUNCTION CALL
          ASTExpression* function_call = s_parse_function_call(lexer);
          if(function_call && s_lexer_skip_token(lexer, TOKEN_SEMICOLON))
          {
            return mi_ast_statement_create_function_call(function_call);
          }
        }
        else if (look_ahead_token2.type == TOKEN_OP_ASSIGN)
        {
          // ASSIGNMENT
          ASTStatement* statement = s_parse_assignment_statement(lexer);
          if(statement && s_lexer_skip_token(lexer, TOKEN_SEMICOLON))
            return statement;
        }
        break;
      }

    case TOKEN_RETURN:
      {
        ASTStatement* statement = s_parse_return_statement(lexer);
        if(statement && s_lexer_skip_token(lexer, TOKEN_SEMICOLON))
          return statement;
        break;
      }

    case TOKEN_FOR:
      {
        return s_parse_for_statement(lexer);
        break;
      }

    case TOKEN_WHILE:
      {
        return s_parse_while_statement(lexer);
        break;
      }

    case TOKEN_IF:
      return s_parse_if_statement(lexer);

    default:
      if (look_ahead_token1.type == TOKEN_EOF)
      {
        s_lexer_skip_token(lexer, TOKEN_EOF); // consume the look_ahead token
        return NULL;
      }
  }

  return NULL;
}

/*
 * <StatementList> -> <Statement> [ <StatementList> ]
 */
static ASTStatement* s_parse_statement_list(Lexer* lexer)
{
  ASTStatement* first_statement = s_parse_raw(lexer);
  ASTStatement* statement = first_statement;

  while(statement)
  {
    statement->next = s_parse_statement(lexer);
    statement = statement->next;
  }

  return first_statement;
};

void s_lexer_init(Lexer *lexer, const char *buffer)
{
  lexer->buffer = (char *)buffer;
  lexer->position = 0;
  lexer->current_char = buffer[lexer->position]; // Start with the first character
  lexer->next_char = lexer->current_char != 0 ? lexer->buffer[1] : 0;
  lexer->line = 1;
  lexer->column = 1;
  lexer->raw_mode = true;
}

//
// Public functions
//


/*
 * <Program> -> ( <StatementList> )*
 */
ASTProgram* mi_parse_program(const char* buffer)
{
  Lexer lexer;
  s_lexer_init(&lexer, buffer);

  ASTStatement* body = s_parse_statement_list(&lexer);
  return mi_ast_program_create(body);
}

