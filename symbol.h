#ifndef SYMBOL_H
#define SYMBOL_H

#include "ast.h"

/**
 * Symbol Type - data types tracked in the symbol table
 * Matches ValueType from AST but kept separate for semantic analysis
 */
typedef enum {
    SYM_INT,
    SYM_FLOAT,
    SYM_CHAR,
    SYM_STRING
} SymbolType;

/**
 * Symbol Entry - represents a variable in the symbol table
 * Organized as a linked list with scope tracking
 */
typedef struct Symbol {
    char *name;         // Variable name
    SymbolType type;    // Data type
    int scope_level;    // Scope depth (0 = global)
    struct Symbol *next; // Next symbol in list
} Symbol;

// Scope management - enter/exit scopes for blocks, functions, etc.
void sym_enter_scope(void);
void sym_exit_scope(void);

// Symbol table operations - insert and lookup variables
void sym_insert(const char *name, SymbolType type);
Symbol *sym_lookup(const char *name);

// Semantic analysis - type checking and validation
void semantic_check(ASTNode *root);

// Error tracking - count of semantic errors found
extern int semantic_errors;

#endif /* SYMBOL_H */
