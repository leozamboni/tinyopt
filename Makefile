all:
	bison -d parser.y
	flex lexer.l
	gcc -std=c11 -D_GNU_SOURCE -o comp parser.tab.c lex.yy.c main.c ast.c optimizer.c -lm

clean:
	rm -f comp parser.tab.* lex.yy.c