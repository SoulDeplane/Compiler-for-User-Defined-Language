#include <stdio.h>
#include "ast.h"
#include "symbol.h"
#include "codegen.h"
#include "parser.tab.h"

extern ASTNode *root;

/**
 * Nova Compiler Main Entry Point
 * 
 * Compilation Pipeline:
 * 1. Parsing (via yyparse) - builds AST from source code
 * 2. Semantic Analysis - type checking and validation
 * 3. Code Generation - produces 8086 assembly
 */
int main(void) {
    // Step 1: Parse the input program
    if (yyparse() != 0) {
        fprintf(stderr, "Parsing failed\n");
        return 1;
    }
    
    printf("Parsing successful\n");

    // Step 2: Semantic analysis (type checking, undeclared variables, etc.)
    semantic_check(root);
    if (semantic_errors > 0) {
        fprintf(stderr, "Compilation failed due to semantic errors\n");
        free_ast(root);
        return 1;
    }

    // Step 3: Generate 8086 assembly code
    generate_code(root, "output.asm");
    printf("Code generated: output.asm\n");

    // Clean up
    free_ast(root);
    return 0;
}
