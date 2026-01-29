%code requires {
    #include "ast.h"
}

%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

extern int yylex();
extern int line_no;
void yyerror(const char *s);

ASTNode *root;  // Root of the Abstract Syntax Tree
%}

/* Token value types - yylval union */
%union {
    int ival;
    float fval;
    char cval;
    char *sval;
    ASTNode *node;
}

/* Terminal symbols (tokens from lexer) */
%token LET PRINT IF ELSE FOR TO
%token PLUS MINUS MUL DIV POW
%token GT LT GE LE EQ NE
%token ASSIGN
%token LPAREN RPAREN
%token LBRACKET RBRACKET
%token NEWLINE

%token <ival> INT_LITERAL
%token <fval> FLOAT_LITERAL
%token <cval> CHAR_LITERAL
%token <sval> STRING_LITERAL
%token <sval> ID

/* Operator precedence and associativity */
%left PLUS MINUS
%left MUL DIV
%right POW
%nonassoc GT LT GE LE EQ NE

/* Resolve dangling else ambiguity */
%nonassoc IFX
%nonassoc ELSE

%expect 1  /* Standard dangling-else conflict */

/* Non-terminal symbols with AST node types */
%type <node> program stmt_list stmt_list_inner stmt block expr literal

/* Grammar Rules */
%%

/* Program - top-level rule */
program
    : opt_newlines stmt_list opt_newlines
        { root = $2; }
    | opt_newlines
        { root = NULL; }
    ;

/* Statement List - handles multiple statements separated by newlines */
stmt_list
    : stmt_list newline_seq stmt
        { $$ = make_stmt_list($1, $3); }
    | stmt
        { $$ = $1; }
    ;

/* Statement list for inside blocks */
stmt_list_inner
    : stmt_list_inner newline_seq stmt
        { $$ = make_stmt_list($1, $3); }
    | stmt
        { $$ = $1; }
    ;

/* Statements - declarations, print, control flow */
stmt
    : LET ID ASSIGN expr
        { $$ = make_decl($2, $4); }

    | PRINT expr
        { $$ = make_print($2); }

    | IF expr opt_newlines block %prec IFX
        { $$ = make_if($2, $4, NULL); }

    | IF expr opt_newlines block opt_newlines ELSE opt_newlines block
        { $$ = make_if($2, $4, $8); }

    | FOR ID ASSIGN expr TO expr opt_newlines block
        { $$ = make_for($2, $4, $6, $8); }

    | block
        { $$ = $1; }
    ;

/* Block - statements enclosed in brackets with flexible newlines */
block
    : LBRACKET opt_newlines stmt_list_inner opt_newlines RBRACKET
        { $$ = make_block($3); }
    ;

/* Optional newlines */
opt_newlines
    : /* empty */
    | newline_seq
    ;

/* One or more newlines */
newline_seq
    : NEWLINE
    | newline_seq NEWLINE
    ;

/* Expressions - operators, literals, identifiers */
expr
    : expr PLUS expr     { $$ = make_binop('+', $1, $3); }
    | expr MINUS expr    { $$ = make_binop('-', $1, $3); }
    | expr MUL expr      { $$ = make_binop('*', $1, $3); }
    | expr DIV expr      { $$ = make_binop('/', $1, $3); }
    | expr POW expr      { $$ = make_binop('^', $1, $3); }

    | expr GT expr       { $$ = make_binop('>', $1, $3); }
    | expr LT expr       { $$ = make_binop('<', $1, $3); }
    | expr GE expr       { $$ = make_binop('G', $1, $3); }  // Using 'G' for >=
    | expr LE expr       { $$ = make_binop('L', $1, $3); }  // Using 'L' for <=
    | expr EQ expr       { $$ = make_binop('E', $1, $3); }  // Using 'E' for ==
    | expr NE expr       { $$ = make_binop('N', $1, $3); }  // Using 'N' for !=

    | LPAREN expr RPAREN { $$ = $2; }
    | literal            { $$ = $1; }
    | ID                 { $$ = make_id($1); }
    ;

/* Literals - integer, float, char, string */
literal
    : INT_LITERAL        { $$ = make_int($1); }
    | FLOAT_LITERAL      { $$ = make_float($1); }
    | CHAR_LITERAL       { $$ = make_char($1); }
    | STRING_LITERAL     { $$ = make_string($1); }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Parser error at line %d: %s\n", line_no, s);
}
