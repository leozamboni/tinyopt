%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "ast.h"

int yylex(void);
void yyerror(const char *s);

// Variável global para a AST
extern ASTNode *global_ast;

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
    void* node;
}

%token <str> ID NUMBER CHAR_LITERAL STRING_LITERAL
%token INT FLOAT CHAR VOID RETURN
%token IF ELSE WHILE FOR BREAK CONTINUE
%token EQ NE LE GE AND OR
%token INC DEC
%token ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN

%type <node> expression statement declaration assignment
%type <node> condition compound_statement
%type <node> if_statement while_statement for_statement
%type <node> program compound_program

%left AND OR
%left EQ NE
%left '<' '>' LE GE
%left '+' '-'
%left '*' '/' '%'
%right '!'

%%

program:
    program statement { 
        if ($2) {
            add_statement(global_ast, $2);
            printf("Instrução adicionada à AST\n");
        }
    }
    | /* vazio */
    ;

statement:
      declaration ';'         { $$ = $1; }
    | assignment ';'          { $$ = $1; }
    | if_statement            { $$ = $1; }
    | while_statement         { $$ = $1; }
    | for_statement           { $$ = $1; }
    | compound_statement      { $$ = $1; }
    | RETURN expression ';'   { 
        $$ = create_return_node($2);
        printf("return expression\n");
    }
    | BREAK ';'               { 
        $$ = create_control_node(1);
        printf("break\n");
    }
    | CONTINUE ';'            { 
        $$ = create_control_node(0);
        printf("continue\n");
    }
    ;

declaration:
      INT ID         { 
        $$ = create_declaration_node(TYPE_INT, $2, NULL, 0);
        printf("declaração: int %s\n", $2); 
        free($2); 
    }
    | FLOAT ID       { 
        $$ = create_declaration_node(TYPE_FLOAT, $2, NULL, 0);
        printf("declaração: float %s\n", $2); 
        free($2); 
    }
    | CHAR ID        { 
        $$ = create_declaration_node(TYPE_CHAR, $2, NULL, 0);
        printf("declaração: char %s\n", $2); 
        free($2); 
    }
    | VOID ID        { 
        $$ = create_declaration_node(TYPE_VOID, $2, NULL, 0);
        printf("declaração: void %s\n", $2); 
        free($2); 
    }
    | INT ID '=' expression { 
        $$ = create_declaration_node(TYPE_INT, $2, $4, 0);
        printf("declaração: int %s = expression\n", $2); 
        free($2); 
    }
    | FLOAT ID '=' expression { 
        $$ = create_declaration_node(TYPE_FLOAT, $2, $4, 0);
        printf("declaração: float %s = expression\n", $2); 
        free($2); 
    }
    | CHAR ID '=' expression { 
        $$ = create_declaration_node(TYPE_CHAR, $2, $4, 0);
        printf("declaração: char %s = expression\n", $2); 
        free($2); 
    }
    | INT ID '[' NUMBER ']'   { 
        $$ = create_declaration_node(TYPE_ARRAY, $2, NULL, atoi($4));
        printf("declaração: int %s[%s]\n", $2, $4); 
        free($2); 
        free($4); 
    }
    | FLOAT ID '[' NUMBER ']' { 
        $$ = create_declaration_node(TYPE_ARRAY, $2, NULL, atoi($4));
        printf("declaração: float %s[%s]\n", $2, $4); 
        free($2); 
        free($4); 
    }
    ;

assignment:
    ID '=' expression { 
        $$ = create_assignment_node($1, OP_ASSIGN, $3);
        printf("atribuição: %s = expression\n", $1); 
        free($1); 
    }
    | ID ADD_ASSIGN expression { 
        $$ = create_assignment_node($1, OP_ADD_ASSIGN, $3);
        printf("atribuição: %s += expression\n", $1); 
        free($1); 
    }
    | ID SUB_ASSIGN expression { 
        $$ = create_assignment_node($1, OP_SUB_ASSIGN, $3);
        printf("atribuição: %s -= expression\n", $1); 
        free($1); 
    }
    | ID MUL_ASSIGN expression { 
        $$ = create_assignment_node($1, OP_MUL_ASSIGN, $3);
        printf("atribuição: %s *= expression\n", $1); 
        free($1); 
    }
    | ID DIV_ASSIGN expression { 
        $$ = create_assignment_node($1, OP_DIV_ASSIGN, $3);
        printf("atribuição: %s /= expression\n", $1); 
        free($1); 
    }
    | ID MOD_ASSIGN expression { 
        $$ = create_assignment_node($1, OP_MOD_ASSIGN, $3);
        printf("atribuição: %s %%= expression\n", $1); 
        free($1); 
    }
    | ID INC { 
        $$ = create_assignment_node($1, OP_INC, NULL);
        printf("incremento: %s++\n", $1); 
        free($1); 
    }
    | ID DEC { 
        $$ = create_assignment_node($1, OP_DEC, NULL);
        printf("decremento: %s--\n", $1); 
        free($1); 
    }
    | INC ID { 
        $$ = create_assignment_node($2, OP_INC, NULL);
        printf("incremento: ++%s\n", $2); 
        free($2); 
    }
    | DEC ID { 
        $$ = create_assignment_node($2, OP_DEC, NULL);
        printf("decremento: --%s\n", $2); 
        free($2); 
    }
    ;

if_statement:
    IF '(' condition ')' statement { 
        $$ = create_if_node($3, $5, NULL);
        printf("if (condition) then\n");
    }
    | IF '(' condition ')' statement ELSE statement { 
        $$ = create_if_node($3, $5, $7);
        printf("if (condition) then else\n");
    }
    ;

while_statement:
    WHILE '(' condition ')' statement { 
        $$ = create_while_node($3, $5);
        printf("while (condition) do\n");
    }
    ;

for_statement:
    FOR '(' declaration ';' condition ';' assignment ')' statement { 
        $$ = create_for_node($3, $5, $7, $9);
        printf("for (declaração; condition; atribuição) do\n");
    }
    | FOR '(' assignment ';' condition ';' assignment ')' statement { 
        $$ = create_for_node($3, $5, $7, $9);
        printf("for (atribuição; condition; atribuição) do\n");
    }
    | FOR '(' ';' condition ';' assignment ')' statement { 
        $$ = create_for_node(NULL, $4, $6, $8);
        printf("for (; condition; atribuição) do\n");
    }
    | FOR '(' declaration ';' ';' assignment ')' statement { 
        $$ = create_for_node($3, NULL, $6, $8);
        printf("for (declaração; ; atribuição) do\n");
    }
    | FOR '(' assignment ';' ';' assignment ')' statement { 
        $$ = create_for_node($3, NULL, $6, $8);
        printf("for (atribuição; ; atribuição) do\n");
    }
    | FOR '(' ';' ';' assignment ')' statement { 
        $$ = create_for_node(NULL, NULL, $5, $7);
        printf("for (; ; atribuição) do\n");
    }
    | FOR '(' declaration ';' condition ';' ')' statement { 
        $$ = create_for_node($3, $5, NULL, $8);
        printf("for (declaração; condition; ) do\n");
    }
    | FOR '(' assignment ';' condition ';' ')' statement { 
        $$ = create_for_node($3, $5, NULL, $8);
        printf("for (atribuição; condition; ) do\n");
    }
    | FOR '(' ';' condition ';' ')' statement { 
        $$ = create_for_node(NULL, $4, NULL, $7);
        printf("for (; condition; ) do\n");
    }
    | FOR '(' ';' ';' ')' statement { 
        $$ = create_for_node(NULL, NULL, NULL, $6);
        printf("for (; ; ) do\n");
    }
    ;

compound_statement:
    '{' compound_program '}' { 
        ProgramNode *prog = (ProgramNode*)$2;
        $$ = create_compound_node(prog->statements);
        printf("bloco de código\n");
        // Set statements to NULL to avoid double-freeing
        prog->statements = NULL;
    }
    ;

compound_program:
    compound_program statement { 
        if ($2) {
            add_statement($1, $2);
        }
        $$ = $1;
    }
    | /* vazio */
    {
        $$ = create_program_node();
    }
    ;

condition:
      expression EQ expression { 
        printf("Criando condição EQ\n");
        $$ = create_condition_node(OP_EQ, $1, $3);
    }
    | expression NE expression { 
        printf("Criando condição NE\n");
        $$ = create_condition_node(OP_NE, $1, $3);
    }
    | expression '<' expression { 
        printf("Criando condição LT\n");
        $$ = create_condition_node(OP_LT, $1, $3);
    }
    | expression LE expression { 
        printf("Criando condição LE\n");
        $$ = create_condition_node(OP_LE, $1, $3);
    }
    | expression '>' expression { 
        printf("Criando condição GT\n");
        $$ = create_condition_node(OP_GT, $1, $3);
    }
    | expression GE expression { 
        printf("Criando condição GE\n");
        $$ = create_condition_node(OP_GE, $1, $3);
    }
    | condition AND condition { 
        $$ = create_condition_node(OP_AND, $1, $3);
    }
    | condition OR condition { 
        $$ = create_condition_node(OP_OR, $1, $3);
    }
    | '!' condition { 
        $$ = create_unary_op_node(OP_NOT, $2);
    }
    | '(' condition ')' { 
        $$ = $2; 
    }
    | expression { 
        $$ = $1;
    }
    ;

expression:
      expression '+' expression { 
        $$ = create_binary_op_node(OP_ADD, $1, $3);
    }
    | expression '-' expression { 
        $$ = create_binary_op_node(OP_SUB, $1, $3);
    }
    | expression '*' expression { 
        $$ = create_binary_op_node(OP_MUL, $1, $3);
    }
    | expression '/' expression { 
        $$ = create_binary_op_node(OP_DIV, $1, $3);
    }
    | expression '%' expression { 
        $$ = create_binary_op_node(OP_MOD, $1, $3);
    }
    | '(' expression ')' { 
        $$ = $2; 
    }
    | NUMBER { 
        $$ = create_number_node($1, TYPE_INT);
        free($1);
    }
    | CHAR_LITERAL { 
        $$ = create_char_literal_node($1);
        free($1);
    }
    | STRING_LITERAL { 
        $$ = create_string_literal_node($1);
        free($1);
    }
    | ID { 
        $$ = create_identifier_node($1);
        free($1);
    }
    | ID '[' expression ']' { 
        // Simplificado para demonstração
        $$ = create_identifier_node($1);
        free($1);
    }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Erro: %s\n", s);
}
