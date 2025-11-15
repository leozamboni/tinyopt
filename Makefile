CC = gcc
CFLAGS = -std=c99 -D_GNU_SOURCE -Wall -Wextra -I.
CORE_SRC = \
    tinyopt.c \
    tinyopt_ast.c \
    tinyopt_stab.c \
    tinyopt_core.c \
    tinyopt_code.c \
    tinyopt_cfg.c
OPT_SRC = $(wildcard opt/*.c)
GEN_SRC = parser.tab.c lex.yy.c
SRC = $(CORE_SRC) $(OPT_SRC) $(GEN_SRC)
OUT = tinyopt

all: parser lex $(OUT)

parser: parser.y
	bison -d parser.y

lex: lexer.l
	flex lexer.l

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC) -lm

format:
	@indent -gnu *.c *.h opt/*.c opt/*.h

uml:
	@clang-uml
	@plantuml -DPLANTUML_LIMIT_SIZE=16384 -Dscale=2 tinyopt.puml

clean:
	rm -f $(OUT) parser.tab.* lex.yy.c
	rm -f *.c~ *.h~ opt/*.c~ opt/*.h~
	rm -f *.puml