/**
 * @file minima.h
 * @brief This file defines core functions for managing and running Minima programs.
 * 
 * @author marciovmf
 */

#ifndef MINIMA_H
#define MINIMA_H

#include "minima_eval.h"

typedef struct
{
  MiSymbolTable symbols;
  ASTProgram*   ast;
} MiProgram;

MiProgram* mi_program_create(const char* source); // Initializes a `MiProgram` from source code.
int mi_program_run(MiProgram* program);           // Runs a MiProgram
void mi_program_destroy(MiProgram* program);      // Destroys a MiProgram


#endif  // MINIMA_H
