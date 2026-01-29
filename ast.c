#include "ast.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/**
 * Helper: Allocate and initialize a new AST node
 * Uses calloc to ensure all fields are zero-initialized
 */
static ASTNode *new_node(NodeType type) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Fatal error: out of memory\n");
        exit(1);
    }
    node->type = type;
    return node;
}

// ============================================================
// Statement Node Constructors
// ============================================================

ASTNode *make_stmt_list(ASTNode *l, ASTNode *r) {
    ASTNode *node = new_node(NODE_STMT_LIST);
    node->left = l;
    node->right = r;
    return node;
}

ASTNode *make_decl(char *name, ASTNode *expr) {
    ASTNode *node = new_node(NODE_DECL);
    node->name = strdup(name);  // Make a copy of the name
    node->left = expr;
    return node;
}

ASTNode *make_print(ASTNode *expr) {
    ASTNode *node = new_node(NODE_PRINT);
    node->left = expr;
    return node;
}

ASTNode *make_if(ASTNode *cond, ASTNode *body, ASTNode *else_body) {
    ASTNode *node = new_node(NODE_IF);
    node->cond = cond;
    node->body = body;
    node->else_body = else_body;  // Can be NULL for simple if
    return node;
}

ASTNode *make_for(char *var, ASTNode *from, ASTNode *to, ASTNode *body) {
    ASTNode *node = new_node(NODE_FOR);
    node->name = strdup(var);   // Loop variable name
    node->left = from;          // Start value
    node->right = to;           // End value
    node->body = body;
    return node;
}

ASTNode *make_block(ASTNode *stmts) {
    ASTNode *node = new_node(NODE_BLOCK);
    node->body = stmts;
    return node;
}

// ============================================================
// Expression Node Constructors
// ============================================================

ASTNode *make_binop(char op, ASTNode *l, ASTNode *r) {
    ASTNode *node = new_node(NODE_BINOP);
    node->op = op;
    node->left = l;
    node->right = r;
    return node;
}

ASTNode *make_id(char *name) {
    ASTNode *node = new_node(NODE_ID);
    node->name = strdup(name);
    return node;
}

// ============================================================
// Literal Node Constructors
// ============================================================

ASTNode *make_int(int v) {
    ASTNode *node = new_node(NODE_LITERAL);
    node->vtype = TYPE_INT;
    node->ival = v;
    return node;
}

ASTNode *make_float(float v) {
    ASTNode *node = new_node(NODE_LITERAL);
    node->vtype = TYPE_FLOAT;
    node->fval = v;
    return node;
}

ASTNode *make_char(char v) {
    ASTNode *node = new_node(NODE_LITERAL);
    node->vtype = TYPE_CHAR;
    node->cval = v;
    return node;
}

ASTNode *make_string(char *v) {
    ASTNode *node = new_node(NODE_LITERAL);
    node->vtype = TYPE_STRING;
    node->sval = strdup(v);  // Make a copy of the string
    return node;
}

// ============================================================
// Memory Management
// ============================================================

/**
 * Recursively free an AST node and all its children
 * Properly handles all dynamically allocated strings
 */
void free_ast(ASTNode *node) {
    if (!node) return;

    // Free child nodes first, then allocated strings, then the node itself
    switch (node->type) {
        case NODE_STMT_LIST:
        case NODE_BINOP:
            free_ast(node->left);
            free_ast(node->right);
            break;

        case NODE_DECL:
            free(node->name);
            free_ast(node->left);
            break;

        case NODE_PRINT:
            free_ast(node->left);
            break;

        case NODE_IF:
            free_ast(node->cond);
            free_ast(node->body);
            free_ast(node->else_body);
            break;

        case NODE_FOR:
            free(node->name);
            free_ast(node->left);
            free_ast(node->right);
            free_ast(node->body);
            break;

        case NODE_BLOCK:
            free_ast(node->body);
            break;

        case NODE_ID:
            free(node->name);
            break;

        case NODE_LITERAL:
            if (node->vtype == TYPE_STRING)
                free(node->sval);
            break;
    }

    free(node);
}

// ============================================================
// Debug / Visualization
// ============================================================

// Helper: print indentation for tree visualization
static void indent_print(int level) {
    for (int i = 0; i < level; i++)
        printf("  ");
}

/**
 * Print AST in a tree structure for debugging
 * Recursively prints each node with indentation showing tree depth
 */
void print_ast(ASTNode *node, int indent) {
    if (!node) return;

    indent_print(indent);

    switch (node->type) {
        case NODE_STMT_LIST:
            printf("STMT_LIST\n");
            print_ast(node->left, indent + 1);
            print_ast(node->right, indent + 1);
            break;

        case NODE_DECL:
            printf("DECL %s\n", node->name);
            print_ast(node->left, indent + 1);
            break;

        case NODE_PRINT:
            printf("PRINT\n");
            print_ast(node->left, indent + 1);
            break;

        case NODE_IF:
            printf("IF\n");
            indent_print(indent + 1);
            printf("COND\n");
            print_ast(node->cond, indent + 2);
            indent_print(indent + 1);
            printf("BODY\n");
            print_ast(node->body, indent + 2);
            if (node->else_body) {
                indent_print(indent + 1);
                printf("ELSE\n");
                print_ast(node->else_body, indent + 2);
            }
            break;

        case NODE_FOR:
            printf("FOR %s\n", node->name);
            indent_print(indent + 1);
            printf("FROM\n");
            print_ast(node->left, indent + 2);
            indent_print(indent + 1);
            printf("TO\n");
            print_ast(node->right, indent + 2);
            indent_print(indent + 1);
            printf("BODY\n");
            print_ast(node->body, indent + 2);
            break;

        case NODE_BLOCK:
            printf("BLOCK\n");
            print_ast(node->body, indent + 1);
            break;

        case NODE_BINOP:
            printf("BINOP '%c'\n", node->op);
            print_ast(node->left, indent + 1);
            print_ast(node->right, indent + 1);
            break;

        case NODE_ID:
            printf("ID %s\n", node->name);
            break;

        case NODE_LITERAL:
            switch (node->vtype) {
                case TYPE_INT:
                    printf("INT %d\n", node->ival);
                    break;
                case TYPE_FLOAT:
                    printf("FLOAT %f\n", node->fval);
                    break;
                case TYPE_CHAR:
                    printf("CHAR '%c'\n", node->cval);
                    break;
                case TYPE_STRING:
                    printf("STRING \"%s\"\n", node->sval);
                    break;
            }
            break;
    }
}
