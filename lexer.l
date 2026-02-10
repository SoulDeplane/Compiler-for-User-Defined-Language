%option noyywrap
%option noinput
%option nounput

%{
#include "parser.tab.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int line_no = 1;
%}


DIGIT      [0-9]
ID         [a-zA-Z_][a-zA-Z0-9_]*
INT        {DIGIT}+
FLOAT      {DIGIT}+"."{DIGIT}+
CHAR       \'([^\\']|\\.)\'
STRING     \"([^\\\"]|\\.)*\"

%%
"let"       { return LET; }
"print"     { return PRINT; }
"if"        { return IF; }
"else"      { return ELSE; }
"for"       { return FOR; }
"to"        { return TO; }
"+"         { return PLUS; }
"-"         { return MINUS; }
"*"         { return MUL; }
"/"         { return DIV; }
"^"         { return POW; }
">="        { return GE; }
"<="        { return LE; }
"=="        { return EQ; }
"!="        { return NE; }
">"         { return GT; }
"<"         { return LT; }
"="         { return ASSIGN; }
"("         { return LPAREN; }
")"         { return RPAREN; }
"["         { return LBRACKET; }
"]"         { return RBRACKET; }
{FLOAT} {
    yylval.fval = atof(yytext);
    return FLOAT_LITERAL;
}
{INT} {
    yylval.ival = atoi(yytext);
    return INT_LITERAL;
}
{CHAR} {
    char ch = yytext[1];
    if (ch == '\\') {
        switch (yytext[2]) {
            case 'n': ch = '\n'; break;
            case 't': ch = '\t'; break;
            case 'r': ch = '\r'; break;
            case '0': ch = '\0'; break;
            case '\\': ch = '\\'; break;
            case '\'': ch = '\''; break;
            default: ch = yytext[2];
        }
    }
    yylval.cval = ch;
    return CHAR_LITERAL;
}
{STRING} {
    int len = strlen(yytext);
    char *str = malloc(len);
    int j = 0;
    for (int i = 1; i < len - 1; i++) {
        if (yytext[i] == '\\') {
            i++;
            switch (yytext[i]) {
                case 'n': str[j++] = '\n'; break;
                case 't': str[j++] = '\t'; break;
                case 'r': str[j++] = '\r'; break;
                case '\\': str[j++] = '\\'; break;
                case '"': str[j++] = '"'; break;
                default: str[j++] = yytext[i];
            }
        } else {
            str[j++] = yytext[i];
        }
    }
    str[j] = '\0';
    yylval.sval = str;
    return STRING_LITERAL;
}
{ID} {
    yylval.sval = strdup(yytext);
    return ID;
}
\n {
    line_no++;
    return NEWLINE;
}
[ \t\r]+ ;
\$[^\n]* ;
"<"([^>\n]|[\n])*">" {
    for (int i = 0; i < yyleng; i++) {
        if (yytext[i] == '\n')
            line_no++;
    }
}
. {
    fprintf(stderr,
        "Lexical error at line %d: unrecognized character '%s'\n",
        line_no, yytext);
}
%%
