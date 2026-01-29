#ifndef AST_H
#define AST_H

/**
 * AST Node Types - represents different statement and expression types
 */
typedef enum {
    NODE_STMT_LIST,   // List of statements
    NODE_DECL,        // Variable declaration
    NODE_PRINT,       // Print statement
    NODE_IF,          // If/else conditional
    NODE_FOR,         // For loop
    NODE_BLOCK,       // Block of statements
    NODE_BINOP,       // Binary operation (+, -, *, /, etc.)
    NODE_LITERAL,     // Literal value (int, float, char, string)
    NODE_ID           // Variable identifier
} NodeType;

/**
 * Value Types - represents data types for literals and variables
 */
typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_CHAR,
    TYPE_STRING
} ValueType;

/**
 * AST Node Structure - flexible node that can represent any syntax tree element
 * 
 * Different fields are used depending on the node type:
 * - BINOP: op, left, right
 * - DECL/ID: name, left (for expression)
 * - LITERAL: vtype, ival/fval/cval/sval
 * - IF: cond, body, else_body
 * - FOR: name, left (from), right (to), body
 */
typedef struct ASTNode {
    NodeType type;
    ValueType vtype;

    char op;          // Binary operator (+, -, *, /, etc.)
    char *name;       // Variable name (for ID, DECL, FOR)

    // Literal values (only one is used based on vtype)
    int ival;
    float fval;
    char cval;
    char *sval;

    // Tree structure (used for expressions and statement lists)
    struct ASTNode *left;
    struct ASTNode *right;

    // Control flow (used for IF, FOR, BLOCK)
    struct ASTNode *cond;
    struct ASTNode *body;
    struct ASTNode *else_body;
} ASTNode;

// Statement constructors
ASTNode *make_stmt_list(ASTNode *l, ASTNode *r);
ASTNode *make_decl(char *name, ASTNode *expr);
ASTNode *make_print(ASTNode *expr);
ASTNode *make_if(ASTNode *cond, ASTNode *body, ASTNode *else_body);
ASTNode *make_for(char *var, ASTNode *from, ASTNode *to, ASTNode *body);
ASTNode *make_block(ASTNode *stmts);

// Expression constructors
ASTNode *make_binop(char op, ASTNode *l, ASTNode *r);
ASTNode *make_id(char *name);

// Literal constructors
ASTNode *make_int(int v);
ASTNode *make_float(float v);
ASTNode *make_char(char v);
ASTNode *make_string(char *v);

// Memory management
void free_ast(ASTNode *node);

// Debug utilities
void print_ast(ASTNode *node, int indent);

#endif /* AST_H */
