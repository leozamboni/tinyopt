%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

int yylex(void);
void yyerror(const char *s);

// Função asprintf para compatibilidade
int asprintf(char **strp, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int size = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    
    if (size < 0) return -1;
    
    *strp = malloc(size + 1);
    if (!*strp) return -1;
    
    va_start(args, fmt);
    int result = vsnprintf(*strp, size + 1, fmt, args);
    va_end(args);
    
    return result;
}

%}

%union {
    char* str;
}

%token <str> ID NUMBER CHAR_LITERAL STRING_LITERAL
%token INT FLOAT CHAR VOID RETURN
%token IF ELSE WHILE FOR BREAK CONTINUE
%token EQ NE LE GE AND OR
%token INC DEC
%token ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN

%type <str> expression statement declaration assignment
%type <str> condition compound_statement
%type <str> if_statement while_statement for_statement

%left AND OR
%left EQ NE
%left '<' '>' LE GE
%left '+' '-'
%left '*' '/' '%'
%right '!'

%%

program:
    program statement
    | /* vazio */
    ;

statement:
      declaration ';'
    | assignment ';'
    | if_statement
    | while_statement
    | for_statement
    | compound_statement
    | RETURN expression ';'   { printf("return %s\n", $2); free($2); }
    | BREAK ';'               { printf("break\n"); }
    | CONTINUE ';'            { printf("continue\n"); }
    ;

declaration:
      INT ID         { printf("declaração: int %s\n", $2); free($2); }
    | FLOAT ID       { printf("declaração: float %s\n", $2); free($2); }
    | CHAR ID        { printf("declaração: char %s\n", $2); free($2); }
    | VOID ID        { printf("declaração: void %s\n", $2); free($2); }
    | INT ID '=' expression { printf("declaração: int %s = %s\n", $2, $4); free($2); free($4); }
    | FLOAT ID '=' expression { printf("declaração: float %s = %s\n", $2, $4); free($2); free($4); }
    | CHAR ID '=' expression { printf("declaração: char %s = %s\n", $2, $4); free($2); free($4); }
    | INT ID '[' NUMBER ']'   { printf("declaração: int %s[%s]\n", $2, $4); free($2); free($4); }
    | FLOAT ID '[' NUMBER ']' { printf("declaração: float %s[%s]\n", $2, $4); free($2); free($4); }
    ;

assignment:
    ID '=' expression { printf("atribuição: %s = %s\n", $1, $3); free($1); free($3); }
    | ID ADD_ASSIGN expression { printf("atribuição: %s += %s\n", $1, $3); free($1); free($3); }
    | ID SUB_ASSIGN expression { printf("atribuição: %s -= %s\n", $1, $3); free($1); free($3); }
    | ID MUL_ASSIGN expression { printf("atribuição: %s *= %s\n", $1, $3); free($1); free($3); }
    | ID DIV_ASSIGN expression { printf("atribuição: %s /= %s\n", $1, $3); free($1); free($3); }
    | ID MOD_ASSIGN expression { printf("atribuição: %s %%= %s\n", $1, $3); free($1); free($3); }
    | ID INC                     { printf("incremento: %s++\n", $1); free($1); }
    | ID DEC                     { printf("decremento: %s--\n", $1); free($1); }
    | INC ID                     { printf("incremento: ++%s\n", $2); free($2); }
    | DEC ID                     { printf("decremento: --%s\n", $2); free($2); }
    ;

if_statement:
    IF '(' condition ')' statement { printf("if (%s) then\n", $3); free($3); }
    | IF '(' condition ')' statement ELSE statement { printf("if (%s) then else\n", $3); free($3); }
    ;

while_statement:
    WHILE '(' condition ')' statement { printf("while (%s) do\n", $3); free($3); }
    ;

for_statement:
    FOR '(' declaration ';' condition ';' assignment ')' statement { 
        printf("for (declaração; %s; atribuição) do\n", $5); 
        free($5); 
        $$ = strdup("for_loop");
    }
    | FOR '(' assignment ';' condition ';' assignment ')' statement { 
        printf("for (atribuição; %s; atribuição) do\n", $5); 
        free($5); 
        $$ = strdup("for_loop");
    }
    | FOR '(' ';' condition ';' assignment ')' statement { 
        printf("for (; %s; atribuição) do\n", $4); 
        free($4); 
        $$ = strdup("for_loop");
    }
    | FOR '(' declaration ';' ';' assignment ')' statement { 
        printf("for (declaração; ; atribuição) do\n"); 
        $$ = strdup("for_loop");
    }
    | FOR '(' assignment ';' ';' assignment ')' statement { 
        printf("for (atribuição; ; atribuição) do\n"); 
        $$ = strdup("for_loop");
    }
    | FOR '(' ';' ';' assignment ')' statement { 
        printf("for (; ; atribuição) do\n"); 
        $$ = strdup("for_loop");
    }
    | FOR '(' declaration ';' condition ';' ')' statement { 
        printf("for (declaração; %s; ) do\n", $5); 
        free($5); 
        $$ = strdup("for_loop");
    }
    | FOR '(' assignment ';' condition ';' ')' statement { 
        printf("for (atribuição; %s; ) do\n", $5); 
        free($5); 
        $$ = strdup("for_loop");
    }
    | FOR '(' ';' condition ';' ')' statement { 
        printf("for (; %s; ) do\n", $4); 
        free($4); 
        $$ = strdup("for_loop");
    }
    | FOR '(' ';' ';' ')' statement { 
        printf("for (; ; ) do\n"); 
        $$ = strdup("for_loop");
    }
    ;

compound_statement:
    '{' program '}' { printf("bloco de código\n"); }
    ;

condition:
      expression EQ expression { asprintf(&$$, "(%s == %s)", $1, $3); free($1); free($3); }
    | expression NE expression { asprintf(&$$, "(%s != %s)", $1, $3); free($1); free($3); }
    | expression '<' expression { asprintf(&$$, "(%s < %s)", $1, $3); free($1); free($3); }
    | expression LE expression { asprintf(&$$, "(%s <= %s)", $1, $3); free($1); free($3); }
    | expression '>' expression { asprintf(&$$, "(%s > %s)", $1, $3); free($1); free($3); }
    | expression GE expression { asprintf(&$$, "(%s >= %s)", $1, $3); free($1); free($3); }
    | condition AND condition { asprintf(&$$, "(%s && %s)", $1, $3); free($1); free($3); }
    | condition OR condition { asprintf(&$$, "(%s || %s)", $1, $3); free($1); free($3); }
    | '!' condition { asprintf(&$$, "(!%s)", $2); free($2); }
    | '(' condition ')' { $$ = $2; }
    | expression
    ;

expression:
      expression '+' expression { asprintf(&$$, "(%s + %s)", $1, $3); free($1); free($3); }
    | expression '-' expression { asprintf(&$$, "(%s - %s)", $1, $3); free($1); free($3); }
    | expression '*' expression { asprintf(&$$, "(%s * %s)", $1, $3); free($1); free($3); }
    | expression '/' expression { asprintf(&$$, "(%s / %s)", $1, $3); free($1); free($3); }
    | expression '%' expression { asprintf(&$$, "(%s %% %s)", $1, $3); free($1); free($3); }
    | '(' expression ')' { $$ = $2; }
    | NUMBER                   { $$ = $1; }
    | CHAR_LITERAL             { $$ = $1; }
    | STRING_LITERAL           { $$ = $1; }
    | ID                       { $$ = $1; }
    | ID '[' expression ']'    { asprintf(&$$, "%s[%s]", $1, $3); free($1); free($3); }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Erro: %s\n", s);
}
