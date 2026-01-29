#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// ============================================================
// Global State
// ============================================================

static FILE *out;        // Output file for assembly code
static int label_id = 0; // Counter for generating unique labels

// ============================================================
// Assembly Emission
// ============================================================

// Helper: emit a line of assembly code with printf-style formatting
static void emit(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(out, fmt, args);
    fprintf(out, "\n");
    va_end(args);
}

// ============================================================
// Variable Table - tracks all variables for data segment
// ============================================================

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

// Add a variable to the table (avoid duplicates)
static void add_var(const char *name) {
    if (var_exists(name)) return;
    Var *v = malloc(sizeof(Var));
    v->name = strdup(name);
    v->next = vars;
    vars = v;
}

// ============================================================
// String Table - tracks all string literals for data segment
// ============================================================

typedef struct Str {
    char *label;  // Assembly label (e.g., "STR_0")
    char *value;  // String content
    struct Str *next;
} Str;

static Str *strings = NULL;
static int str_id = 0;

// Add a string to the table with a unique label (avoid duplicates)
static void add_string(const char *s) {
    for (Str *p = strings; p; p = p->next)
        if (strcmp(p->value, s) == 0) return;  // Already exists

    Str *n = malloc(sizeof(Str));
    char buf[32];
    sprintf(buf, "STR_%d", str_id++);
    n->label = strdup(buf);
    n->value = strdup(s);
    n->next = strings;
    strings = n;
}

// Find the assembly label for a string literal
static char *find_string(const char *s) {
    for (Str *p = strings; p; p = p->next)
        if (strcmp(p->value, s) == 0)
            return p->label;
    return NULL;
}

// ============================================================
// Constant Folding Optimization
// ============================================================

/**
 * Perform compile-time constant folding for integer arithmetic
 * Example: 2 + 3 becomes 5 at compile time
 * This reduces the generated assembly code size and improves performance
 */
static ASTNode *fold(ASTNode *n) {
    if (!n || n->type != NODE_BINOP) return n;

    // Recursively fold children first
    n->left = fold(n->left);
    n->right = fold(n->right);

    // Check if both operands are integer constants
    if (n->left->type == NODE_LITERAL &&
        n->right->type == NODE_LITERAL &&
        n->left->vtype == TYPE_INT &&
        n->right->vtype == TYPE_INT) {

        int a = n->left->ival;
        int b = n->right->ival;
        int r;

        // Compute the result at compile time
        switch (n->op) {
            case '+': r = a + b; break;
            case '-': r = a - b; break;
            case '*': r = a * b; break;
            case '/': if (b == 0) return n; r = a / b; break;  // Avoid division by zero
            default: return n;
        }

        // Replace the entire operation with a single literal
        ASTNode *lit = make_int(r);
        free_ast(n);
        return lit;
    }
    return n;
}

// ============================================================
// AST Collection - gather variables and strings before codegen
// ============================================================

/**
 * Single-pass AST traversal to collect:
 * - All variable names (for data segment declarations)
 * - All string literals (for data segment with labels)
 * This must be done before code generation to know what to declare
 */
static void collect_data(ASTNode *n) {
    if (!n) return;

    switch (n->type) {
        case NODE_DECL:
            add_var(n->name);      // Variable declaration
            collect_data(n->left); // Check initializer for strings
            break;

        case NODE_FOR:
            add_var(n->name);       // Loop variable
            collect_data(n->left);  // From expression
            collect_data(n->right); // To expression
            collect_data(n->body);  // Loop body
            break;

        case NODE_LITERAL:
            // Collect string literals for data segment
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

// ============================================================
// 8086 Assembly Generation - Expressions
// ============================================================

/**
 * Generate 8086 assembly code for an expression
 * Result is left in AX register
 * Uses stack for managing binary operation operands
 */
static void gen_expr(ASTNode *n) {
    if (!n) return;

    n = fold(n);  // Apply constant folding optimization first

    switch (n->type) {
        case NODE_LITERAL:
            // Load immediate value into AX
            emit("    mov ax, %d", n->ival);
            break;

        case NODE_ID:
            // Load variable value from memory into AX
            emit("    mov ax, [%s]", n->name);
            break;

        case NODE_BINOP:
            // Evaluate left operand (result in AX)
            gen_expr(n->left);
            emit("    push ax");      // Save left operand on stack
            gen_expr(n->right);       // Evaluate right operand (result in AX)
            emit("    pop bx");       // Retrieve left operand into BX

            // Perform operation (left=BX, right=AX, result=AX)
            switch (n->op) {
                case '+': emit("    add ax, bx"); break;  // AX = AX + BX
                case '-': 
                    // Subtraction: want BX - AX
                    emit("    sub bx, ax");  // BX = BX - AX
                    emit("    mov ax, bx");  // Move result to AX
                    break;
                case '*': emit("    mul bx"); break;  // AX = AX * BX
                case '/':
                    // Division: need to clear DX before DIV
                    emit("    xor dx, dx");  // Clear DX
                    emit("    div bx");      // AX = DX:AX / BX
                    break;
            }
            break;
        default:
            break;
    }
}

// ============================================================
// 8086 Assembly Generation - Statements
// ============================================================

/**
 * Generate 8086 assembly code for statements
 * Handles variable assignment, printing, conditionals, and loops
 */
static void gen_stmt(ASTNode *n) {
    if (!n) return;

    switch (n->type) {
        case NODE_STMT_LIST:
            // Process statements in sequence
            gen_stmt(n->left);
            gen_stmt(n->right);
            break;

        case NODE_DECL:
            // Variable declaration: evaluate expression and store
            gen_expr(n->left);
            emit("    mov [%s], ax", n->name);
            break;

        case NODE_PRINT:
            // Print either a string or an integer
            if (n->left->type == NODE_LITERAL &&
                n->left->vtype == TYPE_STRING) {
                // String printing using DOS interrupt
                char *lbl = find_string(n->left->sval);
                emit("    mov dx, offset %s", lbl);
                emit("    mov ah, 09h");  // DOS print string function
                emit("    int 21h");
            } else {
                // Integer printing using helper function
                gen_expr(n->left);
                emit("    call print_int");
            }
            break;

        case NODE_IF: {
            // If-else: jump to false branch if condition is zero
            int id = label_id++;
            char f[32], e[32];
            sprintf(f, "IF_FALSE_%d", id);
            sprintf(e, "IF_END_%d", id);

            gen_expr(n->cond);            // Evaluate condition
            emit("    cmp ax, 0");       // Check if zero (false)
            emit("    je %s", f);         // Jump to false branch if zero
            gen_stmt(n->body);            // Generate true branch
            emit("    jmp %s", e);        // Skip false branch
            emit("%s:", f);               // False branch label
            gen_stmt(n->else_body);       // Generate false branch
            emit("%s:", e);               // End label
            break;
        }

        case NODE_FOR: {
            // For loop: initialize, check, body, increment
            int id = label_id++;
            char s[32], e[32];
            sprintf(s, "FOR_%d", id);
            sprintf(e, "END_FOR_%d", id);

            gen_expr(n->left);             // Initialize loop variable
            emit("    mov [%s], ax", n->name);

            emit("%s:", s);                // Loop start label
            emit("    mov ax, [%s]", n->name);     // Load loop variable
            emit("    cmp ax, %d", n->right->ival); // Compare with end value
            emit("    jg %s", e);          // Exit if greater

            gen_stmt(n->body);             // Generate loop body

            emit("    inc word ptr [%s]", n->name); // Increment loop variable
            emit("    jmp %s", s);         // Jump back to start
            emit("%s:", e);                // End label
            break;
        }

        case NODE_BLOCK:
            gen_stmt(n->body);
            break;

        default:
            break;
    }
}

// ============================================================
// Main Code Generation Entry Point
// ============================================================

/**
 * Generate complete 8086 assembly program from AST
 * 
 * Steps:
 * 1. Collect all variables and strings from AST
 * 2. Emit .data segment with variable and string declarations
 * 3. Emit .code segment with main program logic
 * 4. Include helper functions (e.g., print_int)
 */
void generate_code(ASTNode *root, const char *outfile) {
    out = fopen(outfile, "w");
    if (!out) exit(1);

    // Initialize state
    vars = NULL;
    strings = NULL;
    label_id = 0;
    str_id = 0;

    // Collect all variables and strings in one pass
    collect_data(root);

    // Emit 8086 assembly header
    emit(".model small");
    emit(".stack 100h");
    emit(".data");

    // Declare all variables (word-sized, uninitialized)
    for (Var *v = vars; v; v = v->next)
        emit("%s dw ?", v->name);

    // Declare all string literals (null-terminated with DOS $)
    for (Str *s = strings; s; s = s->next)
        emit("%s db \"%s$\"", s->label, s->value);

    // Start code segment
    emit(".code");
    emit("main proc");
    emit("    mov ax, 0003h");  // Clear screen (80x25 text mode)
    emit("    int 10h");

    // Generate main program code
    gen_stmt(root);

    // Exit program
    emit("    mov ah, 4Ch");    // DOS exit function
    emit("    int 21h");
    emit("main endp");

    // Helper function: print integer in AX as decimal
    emit("print_int proc");
    emit("    mov bx, 10");      // Divisor for decimal conversion
    emit("    xor cx, cx");      // Digit counter
    emit("L1:");
    emit("    xor dx, dx");      // Clear DX for division
    emit("    div bx");          // AX = AX / 10, DX = remainder
    emit("    push dx");         // Push digit onto stack
    emit("    inc cx");          // Count digits
    emit("    test ax, ax");     // Check if done
    emit("    jnz L1");          // Continue if more digits
    emit("L2:");
    emit("    pop dx");          // Pop digit from stack
    emit("    add dl, '0'");     // Convert to ASCII
    emit("    mov ah, 02h");     // DOS print character
    emit("    int 21h");
    emit("    loop L2");         // Repeat for all digits
    emit("    ret");
    emit("print_int endp");

    emit("end main");
    fclose(out);
}
