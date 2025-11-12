CC = gcc
CFLAGS = -std=c99 -D_GNU_SOURCE -Wall -Wextra
SRC = parser.tab.c lex.yy.c tinyopt.c tinyopt_ast.c tinyopt_stab.c tinyopt_core.c tinyopt_code.c tinyopt_cfg.c
OUT = tinyopt

all: parser lex $(OUT)

parser: parser.y
	bison -d parser.y

lex: lexer.l
	flex lexer.l

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC) -lm

format:
	@indent -gnu *.c *.h 

clean:
	rm -f $(OUT) parser.tab.* lex.yy.c
	rm -f *.c~ *.h~ 
