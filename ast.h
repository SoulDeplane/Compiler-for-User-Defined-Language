#ifndef AST_H
#define AST_H


typedef enum {
    NODE_STMT_LIST,
    NODE_DECL,
    NODE_PRINT,
    NODE_IF,
    NODE_FOR,
    NODE_BLOCK,
    NODE_BINOP,
    NODE_LITERAL,
    NODE_ID
} NodeType;


typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_CHAR,
    TYPE_STRING
} ValueType;


typedef struct ASTNode {
    NodeType type;
    ValueType vtype;

    char op;
    char *name;

    int ival;
    float fval;
    char cval;
    char *sval;

    struct ASTNode *left;
    struct ASTNode *right;

    struct ASTNode *cond;
    struct ASTNode *body;
    struct ASTNode *else_body;
} ASTNode;


ASTNode *make_stmt_list(ASTNode *l, ASTNode *r);
ASTNode *make_decl(char *name, ASTNode *expr);
ASTNode *make_print(ASTNode *expr);
ASTNode *make_if(ASTNode *cond, ASTNode *body, ASTNode *else_body);
ASTNode *make_for(char *var, ASTNode *from, ASTNode *to, ASTNode *body);
ASTNode *make_block(ASTNode *stmts);


ASTNode *make_binop(char op, ASTNode *l, ASTNode *r);
ASTNode *make_id(char *name);


ASTNode *make_int(int v);
ASTNode *make_float(float v);
ASTNode *make_char(char v);
ASTNode *make_string(char *v);


void free_ast(ASTNode *node);


void print_ast(ASTNode *node, int indent);


/* --- From symbol.h --- */

typedef enum {
    SYM_INT,
    SYM_FLOAT,
    SYM_CHAR,
    SYM_STRING
} SymbolType;


typedef struct Symbol {
    char *name;
    SymbolType type;
    int scope_level;
    struct Symbol *next;
} Symbol;


void sym_enter_scope(void);
void sym_exit_scope(void);


void sym_insert(const char *name, SymbolType type);
Symbol *sym_lookup(const char *name);


void semantic_check(ASTNode *root);


extern int semantic_errors;

/* --- From codegen.h --- */

void generate_code(ASTNode *root, const char *outfile);

#endif /* AST_H */
