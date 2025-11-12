%{
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "tinyopt_ast.h"
#include "parser.tab.h"

uint64_t fnv1a_with_salt(const char *s) {
    uint64_t hash = 14695981039346656037ULL;
    unsigned long salt = (unsigned long)rand(); 
    hash ^= salt;
    while (*s) {
        hash ^= (unsigned char)(*s++);
        hash *= 1099511628211ULL;
    }
    return hash;
}
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
"while"         { yylval.hash = fnv1a_with_salt(yytext); return WHILE; }
"for"           { yylval.hash = fnv1a_with_salt(yytext); return FOR; }
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
                    yylval.identifier.hash = fnv1a_with_salt(yytext);
                    yylval.identifier.name = strdup(yytext);
                    return ID;
                }

"+"             { return '+'; }
"-"             { return '-'; }
"*"             { return '*'; }
"/"             { return '/'; }
"%"             { return '%'; }
"="             { return '='; }
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
