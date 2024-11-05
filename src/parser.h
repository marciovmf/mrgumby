#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include <stdbool.h>

ASTProgram* parse_program(const char *buffer);

#endif // PARSER_H
