#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static FILE *out;
static int label_id = 0;




static void emit(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(out, fmt, args);
    fprintf(out, "\n");
    va_end(args);
}



typedef struct Var {
    char *name;
    struct Var *next;
} Var;

static Var *vars = NULL;

static int var_exists(const char *name) {
    for (Var *v = vars; v; v = v->next)
        if (strcmp(v->name, name) == 0) return 1;
    return 0;
}

static void add_var(const char *name) {
    if (var_exists(name)) return;
    Var *v = malloc(sizeof(Var));
    v->name = strdup(name);
    v->next = vars;
    vars = v;
}

typedef struct Str {
    char *label;
    char *value;
    struct Str *next;
} Str;

static Str *strings = NULL;
static int str_id = 0;

static void add_string(const char *s) {
    for (Str *p = strings; p; p = p->next)
        if (strcmp(p->value, s) == 0) return;

    Str *n = malloc(sizeof(Str));
    char buf[32];
    sprintf(buf, "STR_%d", str_id++);
    n->label = strdup(buf);
    n->value = strdup(s);
    n->next = strings;
    strings = n;
}

static char *find_string(const char *s) {
    for (Str *p = strings; p; p = p->next)
        if (strcmp(p->value, s) == 0)
            return p->label;
    return NULL;
}




static ASTNode *fold(ASTNode *n) {
    if (!n || n->type != NODE_BINOP) return n;

    n->left = fold(n->left);
    n->right = fold(n->right);
    if (n->left->type == NODE_LITERAL &&
        n->right->type == NODE_LITERAL &&
        n->left->vtype == TYPE_INT &&
        n->right->vtype == TYPE_INT) {

        int a = n->left->ival;
        int b = n->right->ival;
        int r;

        switch (n->op) {
            case '+': r = a + b; break;
            case '-': r = a - b; break;
            case '*': r = a * b; break;
            case '/': if (b == 0) return n; r = a / b; break;
            default: return n;
        }
        ASTNode *lit = make_int(r);
        // free_ast(n); // Prevent double-free since tree structure isn't updated
        return lit;
    }
    return n;
}

static void collect_data(ASTNode *n) {
    if (!n) return;

    switch (n->type) {
        case NODE_DECL:
            add_var(n->name);
            collect_data(n->left);
            break;

        case NODE_FOR:
            add_var(n->name);
            collect_data(n->left);
            collect_data(n->right);
            collect_data(n->body);
            break;

        case NODE_LITERAL:
            if (n->vtype == TYPE_STRING)
                add_string(n->sval);
            break;

        case NODE_BINOP:
            collect_data(n->left);
            collect_data(n->right);
            break;

        case NODE_PRINT:
            collect_data(n->left);
            break;

        case NODE_IF:
            collect_data(n->cond);
            collect_data(n->body);
            collect_data(n->else_body);
            break;

        case NODE_STMT_LIST:
            collect_data(n->left);
            collect_data(n->right);
            break;

        case NODE_BLOCK:
            collect_data(n->body);
            break;

        default:
            break;
    }
}

static void gen_expr(ASTNode *n) {
    if (!n) return;

    if (!n) return;

    n = fold(n);

    switch (n->type) {
        case NODE_LITERAL:
            emit("    mov ax, %d", n->ival);
            break;

        case NODE_ID:
            emit("    mov ax, [%s]", n->name);
            break;

        case NODE_BINOP:
            gen_expr(n->left);
            emit("    push ax");
            gen_expr(n->right);
            emit("    pop bx");

            switch (n->op) {
                case '+': emit("    add ax, bx"); break;
                case '-': 
                    emit("    sub bx, ax");
                    emit("    mov ax, bx");
                    break;
                case '*': emit("    mul bx"); break;
                case '/':
                    emit("    xor dx, dx");
                    emit("    div bx");
                    break;
                case '>': {
                    int l = label_id++;
                    emit("    cmp bx, ax");
                    emit("    jg L_TRUE_%d", l);
                    emit("    mov ax, 0");
                    emit("    jmp L_END_%d", l);
                    emit("L_TRUE_%d:", l);
                    emit("    mov ax, 1");
                    emit("L_END_%d:", l);
                    break;
                }
                case '<': {
                    int l = label_id++;
                    emit("    cmp bx, ax");
                    emit("    jl L_TRUE_%d", l);
                    emit("    mov ax, 0");
                    emit("    jmp L_END_%d", l);
                    emit("L_TRUE_%d:", l);
                    emit("    mov ax, 1");
                    emit("L_END_%d:", l);
                    break;
                }
                case 'G': {
                    int l = label_id++;
                    emit("    cmp bx, ax");
                    emit("    jge L_TRUE_%d", l);
                    emit("    mov ax, 0");
                    emit("    jmp L_END_%d", l);
                    emit("L_TRUE_%d:", l);
                    emit("    mov ax, 1");
                    emit("L_END_%d:", l);
                    break;
                }
                case 'L': {
                    int l = label_id++;
                    emit("    cmp bx, ax");
                    emit("    jle L_TRUE_%d", l);
                    emit("    mov ax, 0");
                    emit("    jmp L_END_%d", l);
                    emit("L_TRUE_%d:", l);
                    emit("    mov ax, 1");
                    emit("L_END_%d:", l);
                    break;
                }
                case 'E': {
                    int l = label_id++;
                    emit("    cmp bx, ax");
                    emit("    je L_TRUE_%d", l);
                    emit("    mov ax, 0");
                    emit("    jmp L_END_%d", l);
                    emit("L_TRUE_%d:", l);
                    emit("    mov ax, 1");
                    emit("L_END_%d:", l);
                    break;
                }
                case 'N': {
                    int l = label_id++;
                    emit("    cmp bx, ax");
                    emit("    jne L_TRUE_%d", l);
                    emit("    mov ax, 0");
                    emit("    jmp L_END_%d", l);
                    emit("L_TRUE_%d:", l);
                    emit("    mov ax, 1");
                    emit("L_END_%d:", l);
                }
            }
            break;
        default:
            break;
    }
}

static void gen_stmt(ASTNode *n) {
    if (!n) return;

    switch (n->type) {
        case NODE_STMT_LIST:
            gen_stmt(n->left);
            gen_stmt(n->right);
            break;

        case NODE_DECL:
            gen_expr(n->left);
            emit("    mov [%s], ax", n->name);
            break;

        case NODE_PRINT:
            if (n->left->type == NODE_LITERAL &&
                n->left->vtype == TYPE_STRING) {
                char *lbl = find_string(n->left->sval);
                emit("    mov dx, offset %s", lbl);
                emit("    mov ah, 09h");
                emit("    int 21h");
            } else {
                gen_expr(n->left);
                emit("    call print_int");
            }
            break;

        case NODE_IF: {
            int id = label_id++;
            char f[32], e[32];
            sprintf(f, "IF_FALSE_%d", id);
            sprintf(e, "IF_END_%d", id);

            gen_expr(n->cond);
            emit("    cmp ax, 0");
            emit("    je %s", f);
            gen_stmt(n->body);
            emit("    jmp %s", e);
            emit("%s:", f);
            gen_stmt(n->else_body);
            emit("%s:", e);
            break;
        }


        case NODE_FOR: {
            int id = label_id++;
            char s[32], e[32];
            sprintf(s, "FOR_%d", id);
            sprintf(e, "END_FOR_%d", id);

            gen_expr(n->left);
            emit("    mov [%s], ax", n->name);

            emit("%s:", s);
            emit("    mov ax, [%s]", n->name);
            emit("    cmp ax, %d", n->right->ival);
            emit("    jg %s", e);

            gen_stmt(n->body);

            emit("    inc word ptr [%s]", n->name);
            emit("    jmp %s", s);
            emit("%s:", e);
            break;
        }

        case NODE_BLOCK:
            gen_stmt(n->body);
            break;

        default:
            break;
    }
}

void generate_code(ASTNode *root, const char *outfile) {
    out = fopen(outfile, "w");
    if (!out) exit(1);


    vars = NULL;
    strings = NULL;
    label_id = 0;
    str_id = 0;

    collect_data(root);

    emit(".model small");
    emit(".stack 100h");
    emit(".data");

    for (Var *v = vars; v; v = v->next)
        emit("%s dw ?", v->name);

    for (Str *s = strings; s; s = s->next)
        emit("%s db \"%s$\"", s->label, s->value);

    emit(".code");
    emit("main proc");
    emit("    mov ax, 0003h");
    emit("    int 10h");

    gen_stmt(root);

    emit("    mov ah, 4Ch");
    emit("    int 21h");
    emit("main endp");

    emit("print_int proc");
    emit("    mov bx, 10");
    emit("    xor cx, cx");
    emit("L1:");
    emit("    xor dx, dx");
    emit("    div bx");
    emit("    push dx");
    emit("    inc cx");
    emit("    test ax, ax");
    emit("    jnz L1");
    emit("L2:");
    emit("    pop dx");
    emit("    add dl, '0'");
    emit("    mov ah, 02h");
    emit("    int 21h");
    emit("    loop L2");
    emit("    ret");
    emit("print_int endp");

    emit("end main");
    fclose(out);
}
