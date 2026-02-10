CC = gcc
LEX = flex
YACC = bison
CFLAGS = -Wall

TARGET = nova.exe

SRCS = main.c ast.c symbol.c codegen.c lex.yy.c parser.tab.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

lex.yy.c: lexer.l
	$(LEX) lexer.l

parser.tab.c: parser.y
	$(YACC) -d parser.y

parser.tab.h: parser.tab.c

clean:
	if exist $(TARGET) del $(TARGET)
	if exist lex.yy.c del lex.yy.c
	if exist parser.tab.c del parser.tab.c
	if exist parser.tab.h del parser.tab.h
	if exist parser.output del parser.output
	if exist temp.no del temp.no
	if exist output.asm del output.asm
	if exist __pycache__ rd /s /q __pycache__
