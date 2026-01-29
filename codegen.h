#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"

/* Entry point for 8086 code generation */
void generate_code(ASTNode *root, const char *outfile);

#endif
