#ifndef MINIMASCRIPT_H
#define MINIMASCRIPT_H

#include "minima_ast.h"
#include "minima_eval.h"


typedef ASTProgram MiProgram;

MiProgram*  mi_program_create(const char* source);
int         mi_program_run(MiProgram* program);
void        mi_program_destroy(MiProgram* program);

#endif  // MINIMASCRIPT_H
