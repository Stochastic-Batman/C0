#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "parser.h"  // For decl_t

// Perform semantic analysis on the parsed program AST
void semantic_analyze(decl_t* program);  // This includes name resolution and type checking

#endif
