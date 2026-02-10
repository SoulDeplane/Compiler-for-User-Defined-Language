#include <stdio.h>
#include "ast.h"

#include "parser.tab.h"

extern ASTNode *root;


int main(void) {
    if (yyparse() != 0) {
        fprintf(stderr, "Parsing failed\n");
        return 1;
    }
    
    printf("Lexical analysis successful\n");
    printf("Tokens created\n");
    printf("Syntax analysis successful\n");
    printf("Parse tree created\n");

    semantic_check(root);
    if (semantic_errors > 0) {
        fprintf(stderr, "Compilation failed due to semantic errors\n");
        free_ast(root);
        return 1;
    }

    generate_code(root, "output.asm");
    printf("Code generated: output.asm\n");


    free_ast(root);
    return 0;
}
