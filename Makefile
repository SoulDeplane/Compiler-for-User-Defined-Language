CC = gcc
LEX = flex
YACC = bison
CFLAGS = -Wall

TARGET = nova

SRC = lex.yy.c parser.tab.c ast.c symbol.c codegen.c main.c

# ---------- Default build ----------
all: $(TARGET)

# ---------- Build Nova ----------
$(TARGET): parser lexer
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

# ---------- Generate parser ----------
parser:
	$(YACC) -d parser.y

# ---------- Generate lexer ----------
lexer:
	$(LEX) lexer.l

# ---------- Clean generated files ----------
clean:
	del lex.yy.c parser.tab.c parser.tab.h $(TARGET).exe
