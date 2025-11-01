%{
#include "parser.tab.h"
#include "ast.h"
#include <string.h>
#include <stdlib.h>

int assig_id = 0;
%}

digit       [0-9]
letter      [a-zA-Z_]
id          {letter}({letter}|{digit})*
number      {digit}+(\.{digit}+)?([eE][+-]?{digit}+)?
char_literal \'[^\']\'
string_literal \"[^\"]*\"

%%

"int"           { return INT; }
"float"         { return FLOAT; }
"char"          { return CHAR; }
"void"          { return VOID; }
"return"        { return RETURN; }
"if"            { return IF; }
"else"          { return ELSE; }
"while"         { return WHILE; }
"for"           { return FOR; }
"break"         { return BREAK; }
"continue"      { return CONTINUE; }

{number}        {
                    yylval.str = strdup(yytext);
                    return NUMBER;
                }

{char_literal}  {
                    yylval.str = strdup(yytext);
                    return CHAR_LITERAL;
                }

{string_literal} {
                    yylval.str = strdup(yytext);
                    return STRING_LITERAL;
                }

{id}            {
                    yylval.str = strdup(yytext);
                    return ID;
                }

"+"             { return '+'; }
"-"             { return '-'; }
"*"             { return '*'; }
"/"             { return '/'; }
"%"             { return '%'; }
"="             { assig_id++; return '='; }
"=="            { return EQ; }
"!="            { return NE; }
"<"             { return '<'; }
"<="            { return LE; }
">"             { return '>'; }
">="            { return GE; }
"!"             { return '!'; }
"&&"            { return AND; }
"||"            { return OR; }
"++"            { return INC; }
"--"            { return DEC; }
"+="            { return ADD_ASSIGN; }
"-="            { return SUB_ASSIGN; }
"*="            { return MUL_ASSIGN; }
"/="            { return DIV_ASSIGN; }
"%="            { return MOD_ASSIGN; }

";"             { return ';'; }
","             { return ','; }
"("             { return '('; }
")"             { return ')'; }
"{"             { return '{'; }
"}"             { return '}'; }
"["             { return '['; }
"]"             { return ']'; }

[ \t\r\n]+      { /* ignorar */ }

.               { printf("Caractere inválido: %s\n", yytext); return yytext[0]; }

%%

int yywrap(void) { return 1; }
