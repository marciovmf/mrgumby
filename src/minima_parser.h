#ifndef PARSER_H
#define PARSER_H

#include "minima_ast.h"
#include <stdbool.h>

ASTProgram* mi_parse_program(const char* buffer);

#endif // PARSER_H
