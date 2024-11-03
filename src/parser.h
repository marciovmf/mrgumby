#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include <stdbool.h>

/* Token */


typedef enum TokenType_e
{
  TOKEN_ERROR,              // Represents a tokenizer error
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
  TOKEN_RETURN,             // return keyword
  TOKEN_INCLUDE,            // include keyword
  TOKEN_IDENTIFIER,         // <variable names>
  TOKEN_LITERAL_INT,        // [0-9]+
  TOKEN_LITERAL_FLOAT,      // [0-9]*"."[0-9]+
  TOKEN_LITERAL_STRING,     // "double quote string!"
  TOKEN_EOF,                // End of file/stream
  TOKEN_COUNT_
} TokenType;

#define PARSER_MAX_TOKEN_LENGTH 100
typedef struct
{
  TokenType type;
  char value[PARSER_MAX_TOKEN_LENGTH];
} Token;

void token_clone(const Token* original, Token* token);

/* Lexer */


typedef struct Lexer_t
{
  char *buffer;        // Buffer containing the file contents
  char current_char;   // Current character
  char next_char;      // Next character
  size_t position;     // Current position in the buffer
  int line;            // Current line number
  int column;          // Current column number
} Lexer;


/* Parser */
void lexer_init(Lexer *lexer, const char *buffer);
ASTProgram* parse_program(Lexer *lexer);

#endif // PARSER_H
