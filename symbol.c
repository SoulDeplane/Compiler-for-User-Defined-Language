#include "symbol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================
// Global State
// ============================================================

static Symbol *symbol_table = NULL;  // Head of symbol linked list
static int current_scope = 0;        // Current scope depth (0 = global)
int semantic_errors = 0;             // Public error counter

// ============================================================
// Scope Management
// ============================================================

/**
 * Enter a new scope (e.g., entering a function, block, or loop)
 * Increases scope depth; variables declared will have this scope level
 */
void sym_enter_scope(void) {
    current_scope++;
}

/**
 * Exit current scope - removes all variables declared in this scope
 * Frees memory for scope-local variables as they go out of scope
 */
void sym_exit_scope(void) {
    Symbol **curr = &symbol_table;

    // Walk through list and remove symbols at current scope level
    while (*curr) {
        if ((*curr)->scope_level == current_scope) {
            Symbol *tmp = *curr;
            *curr = tmp->next;  // Unlink from list
            free(tmp->name);
            free(tmp);
        } else {
            curr = &(*curr)->next;  // Move to next
        }
    }
    current_scope--;
}

// ============================================================
// Symbol Table Operations
// ============================================================

/**
 * Look up a symbol in the current scope only
 * Used to detect redeclarations in the same scope
 */
static Symbol *sym_lookup_current_scope(const char *name) {
    for (Symbol *s = symbol_table; s; s = s->next) {
        if (strcmp(s->name, name) == 0 && s->scope_level == current_scope)
            return s;
    }
    return NULL;
}

/**
 * Look up a symbol in any visible scope (current or outer scopes)
 * Returns the first matching symbol found (inner scopes shadow outer)
 */

Symbol *sym_lookup(const char *name) {
    for (Symbol *s = symbol_table; s; s = s->next) {
        if (strcmp(s->name, name) == 0)
            return s;
    }
    return NULL;
}

/**
 * Insert a new symbol into the symbol table
 * Checks for redeclaration in the current scope before inserting
 */
void sym_insert(const char *name, SymbolType type) {
    if (sym_lookup_current_scope(name)) {
        fprintf(stderr, "Semantic error: redeclaration of '%s'\n", name);
        semantic_errors++;
        return;
    }

    // Create new symbol and add to front of list
    Symbol *sym = malloc(sizeof(Symbol));
    sym->name = strdup(name);
    sym->type = type;
    sym->scope_level = current_scope;
    sym->next = symbol_table;
    symbol_table = sym;
}

// ============================================================
// Type Conversion Helpers
// ============================================================

/**
 * Convert AST value type to symbol table type
 * Used when inferring variable types from literal values
 */
static SymbolType ast_to_sym(ValueType v) {
    switch (v) {
        case TYPE_INT: return SYM_INT;
        case TYPE_FLOAT: return SYM_FLOAT;
        case TYPE_CHAR: return SYM_CHAR;
        case TYPE_STRING: return SYM_STRING;
    }
    return SYM_INT;  // Default fallback
}

// Check if a type can be used in arithmetic operations
static int is_numeric(SymbolType t) {
    return t == SYM_INT || t == SYM_FLOAT;
}

// ============================================================
// Semantic Checking - Expression Analysis
// ============================================================

/**
 * Type-check an expression and return its inferred type
 * Validates that:
 * - Variables are declared before use
 * - Binary operations have compatible operand types
 * - Type promotions follow language rules (e.g., int + float = float)
 */
static SymbolType check_expr(ASTNode *node) {
    if (!node) return SYM_INT;

    switch (node->type) {
        case NODE_LITERAL:
            // Literals have their type directly stored
            return ast_to_sym(node->vtype);

        case NODE_ID: {
            // Variables must be declared before use
            Symbol *s = sym_lookup(node->name);
            if (!s) {
                fprintf(stderr,
                    "Semantic error: variable '%s' not declared\n",
                    node->name);
                semantic_errors++;
                return SYM_INT;
            }
            return s->type;
        }

        case NODE_BINOP: {
            // Type-check both operands
            SymbolType l = check_expr(node->left);
            SymbolType r = check_expr(node->right);

            // Arithmetic operators require numeric types
            if (!is_numeric(l) || !is_numeric(r)) {
                fprintf(stderr,
                    "Semantic error: invalid operands for '%c'\n",
                    node->op);
                semantic_errors++;
            }

            // Type promotion: if either operand is float, result is float
            return (l == SYM_FLOAT || r == SYM_FLOAT)
                    ? SYM_FLOAT : SYM_INT;
        }

        default:
            return SYM_INT;
    }
}

// ============================================================
// Semantic Checking - Statement Analysis
// ============================================================

/**
 * Type-check a statement and manage scopes
 * Handles:
 * - Variable declarations (infer type from initializer)
 * - Control flow (if, for)
 * - Scope management for blocks and loops
 */
static void check_stmt(ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_STMT_LIST:
            // Process both statements in sequence
            check_stmt(node->left);
            check_stmt(node->right);
            break;

        case NODE_DECL: {
            // Variable declaration: infer type from initializer expression
            SymbolType t = check_expr(node->left);
            sym_insert(node->name, t);
            break;
        }

        case NODE_PRINT:
            // Just type-check the expression to be printed
            check_expr(node->left);
            break;

        case NODE_BLOCK:
            // Blocks create a new scope for local variables
            sym_enter_scope();
            check_stmt(node->body);
            sym_exit_scope();
            break;

        case NODE_IF:
            // Type-check condition and both branches
            check_expr(node->cond);
            check_stmt(node->body);
            check_stmt(node->else_body);  // May be NULL
            break;

        case NODE_FOR:
            // For loops create a scope containing the loop variable
            sym_enter_scope();
            sym_insert(node->name, SYM_INT);  // Loop var is always int
            check_expr(node->left);   // Check 'from' expression
            check_expr(node->right);  // Check 'to' expression
            check_stmt(node->body);
            sym_exit_scope();
            break;

        default:
            break;
    }
}

/**
 * Main entry point for semantic analysis
 * Resets error counter and symbol table, then checks the entire program
 */
void semantic_check(ASTNode *root) {
    semantic_errors = 0;
    symbol_table = NULL;
    current_scope = 0;

    sym_enter_scope();   // Create global scope
    check_stmt(root);
    sym_exit_scope();

    // Print summary of results
    if (semantic_errors == 0)
        printf("Semantic analysis passed\n");
    else
        printf("Semantic analysis failed (%d errors)\n", semantic_errors);
}
