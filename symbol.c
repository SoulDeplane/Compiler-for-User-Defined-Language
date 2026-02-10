#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Symbol *symbol_table = NULL;
static int current_scope = 0;
int semantic_errors = 0;


void sym_enter_scope(void) {
    current_scope++;
}

void sym_exit_scope(void) {
    Symbol **curr = &symbol_table;

    // Walk through list and remove symbols at current scope level
    while (*curr) {
        if ((*curr)->scope_level == current_scope) {
            Symbol *tmp = *curr;
            *curr = tmp->next;
            free(tmp->name);
            free(tmp);
        } else {
            curr = &(*curr)->next;
        }
    }
    current_scope--;
}


static Symbol *sym_lookup_current_scope(const char *name) {
    for (Symbol *s = symbol_table; s; s = s->next) {
        if (strcmp(s->name, name) == 0 && s->scope_level == current_scope)
            return s;
    }
    return NULL;
}



Symbol *sym_lookup(const char *name) {
    for (Symbol *s = symbol_table; s; s = s->next) {
        if (strcmp(s->name, name) == 0)
            return s;
    }
    return NULL;
}


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


static SymbolType ast_to_sym(ValueType v) {
    switch (v) {
        case TYPE_INT: return SYM_INT;
        case TYPE_FLOAT: return SYM_FLOAT;
        case TYPE_CHAR: return SYM_CHAR;
        case TYPE_STRING: return SYM_STRING;
    }
    return SYM_INT;
}


static int is_numeric(SymbolType t) {
    return t == SYM_INT || t == SYM_FLOAT;
}


static SymbolType check_expr(ASTNode *node) {
    if (!node) return SYM_INT;

    switch (node->type) {
        case NODE_LITERAL:
            return ast_to_sym(node->vtype);

        case NODE_ID: {
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
            SymbolType l = check_expr(node->left);
            SymbolType r = check_expr(node->right);


            if (!is_numeric(l) || !is_numeric(r)) {
                fprintf(stderr,
                    "Semantic error: invalid operands for '%c'\n",
                    node->op);
                semantic_errors++;
            }


            return (l == SYM_FLOAT || r == SYM_FLOAT)
                    ? SYM_FLOAT : SYM_INT;
        }

        default:
            return SYM_INT;
    }
}


static void check_stmt(ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_STMT_LIST:
            check_stmt(node->left);
            check_stmt(node->right);
            break;

        case NODE_DECL: {
            SymbolType t = check_expr(node->left);
            sym_insert(node->name, t);
            break;
        }

        case NODE_PRINT:

            check_expr(node->left);
            break;

        case NODE_BLOCK:
            sym_enter_scope();
            check_stmt(node->body);
            sym_exit_scope();
            break;

        case NODE_IF:
            check_expr(node->cond);
            check_stmt(node->body);
            check_stmt(node->else_body);
            break;

        case NODE_FOR:
            sym_enter_scope();
            sym_insert(node->name, SYM_INT);
            check_expr(node->left);
            check_expr(node->right);
            check_stmt(node->body);
            sym_exit_scope();
            break;

        default:
            break;
    }
}


void semantic_check(ASTNode *root) {
    semantic_errors = 0;
    symbol_table = NULL;
    current_scope = 0;

    sym_enter_scope();
    check_stmt(root);
    sym_exit_scope();


    if (semantic_errors == 0)
        printf("Semantic analysis successful\n");
    else
        printf("Semantic analysis failed (%d errors)\n", semantic_errors);
}
