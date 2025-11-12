%{


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "tinyopt_ast.h"

int yylex(void);
void yyerror(TinyOptASTNode_t *root, const char *s);

// Variável global para a AST
// extern TinyOptASTNode_t *global_ast;

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

%parse-param {TinyOptASTNode_t *root}

%union {
    char* str;
    void* node;
    unsigned long hash;
    struct {
        char* name;
        unsigned long hash;
    } identifier;
}

%token <str> NUMBER CHAR_LITERAL STRING_LITERAL
%token INT FLOAT CHAR VOID RETURN
%token IF ELSE BREAK CONTINUE
%token EQ NE LE GE AND OR
%token INC DEC
%token ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN
%token <hash> WHILE FOR

%type <node> expression statement declaration assignment
%type <node> condition compound_statement
%type <node> if_statement while_statement for_statement
%type <node> program compound_program
%type <node> function_def function_call
%type <node> parameter_list argument_list

%token <identifier> ID

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
            add_statement(root, $2);
        }
    }
    | /* vazio */
    ;

statement:
      declaration ';'         { $$ = $1; }
    | assignment ';'          { $$ = $1; }
    | function_call ';'       { $$ = $1; }
    | if_statement            { $$ = $1; }
    | while_statement         { $$ = $1; }
    | for_statement           { $$ = $1; }
    | compound_statement      { $$ = $1; }
    | function_def            { $$ = $1; }
    | RETURN expression ';'   { 
        $$ = create_return_node($2);
    }
    | BREAK ';'               { 
        $$ = create_control_node(1);
    }
    | CONTINUE ';'            { 
        $$ = create_control_node(0);
    }
    ;

declaration:
      INT ID         { 
        $$ = create_declaration_node(TYPE_INT, $2.name, NULL, 0, $2.hash);
        free($2.name); 
    }
    | FLOAT ID       { 
        $$ = create_declaration_node(TYPE_FLOAT, $2.name, NULL, 0, $2.hash);
        free($2.name); 
    }
    | CHAR ID        { 
        $$ = create_declaration_node(TYPE_CHAR, $2.name, NULL, 0, $2.hash);
        free($2.name); 
    }
    | VOID ID        { 
        $$ = create_declaration_node(TYPE_VOID, $2.name, NULL, 0, $2.hash);
        free($2.name); 
    }
    | INT ID '=' expression { 
        $$ = create_declaration_node(TYPE_INT, $2.name, $4, 0, $2.hash);
        free($2.name); 
    }
    | FLOAT ID '=' expression { 
        $$ = create_declaration_node(TYPE_FLOAT, $2.name, $4, 0, $2.hash);
        free($2.name); 
    }
    | CHAR ID '=' expression { 
        $$ = create_declaration_node(TYPE_CHAR, $2.name, $4, 0, $2.hash);
        free($2.name); 
    }
    | INT ID '[' NUMBER ']'   { 
        $$ = create_declaration_node(TYPE_ARRAY, $2.name, NULL, atoi($4), $2.hash);
        free($2.name); 
        free($4); 
    }
    | FLOAT ID '[' NUMBER ']' { 
        $$ = create_declaration_node(TYPE_ARRAY, $2.name, NULL, atoi($4), $2.hash);
        free($2.name); 
        free($4); 
    }
    ;

assignment:
    ID '=' expression { 
        $$ = create_assignment_node($1.name, OP_ASSIGN, $3, $1.hash);
        free($1.name); 
    }
    | ID ADD_ASSIGN expression { 
        $$ = create_assignment_node($1.name, OP_ADD_ASSIGN, $3, $1.hash);
        free($1.name); 
    }
    | ID SUB_ASSIGN expression { 
        $$ = create_assignment_node($1.name, OP_SUB_ASSIGN, $3, $1.hash);
        free($1.name); 
    }
    | ID MUL_ASSIGN expression { 
        $$ = create_assignment_node($1.name, OP_MUL_ASSIGN, $3, $1.hash);
        free($1.name); 
    }
    | ID DIV_ASSIGN expression { 
        $$ = create_assignment_node($1.name, OP_DIV_ASSIGN, $3, $1.hash);
        free($1.name); 
    }
    | ID MOD_ASSIGN expression { 
        $$ = create_assignment_node($1.name, OP_MOD_ASSIGN, $3, $1.hash);
        free($1.name); 
    }
    | ID INC { 
        $$ = create_assignment_node($1.name, OP_INC, NULL, $1.hash);
        free($1.name); 
    }
    | ID DEC { 
        $$ = create_assignment_node($1.name, OP_DEC, NULL, $1.hash);
        free($1.name); 
    }
    | INC ID { 
        $$ = create_assignment_node($2.name, OP_INC, NULL, $2.hash);
        free($2.name); 
    }
    | DEC ID { 
        $$ = create_assignment_node($2.name, OP_DEC, NULL, $2.hash);
        free($2.name); 
    }
    ;

if_statement:
    IF '(' condition ')' statement { 
        $$ = create_if_node($3, $5, NULL);
    }
    | IF '(' condition ')' statement ELSE statement { 
        $$ = create_if_node($3, $5, $7);
    }
    ;

while_statement:
    WHILE '(' condition ')' statement { 
        $$ = create_while_node($3, $5, $1);
    }
    ;

for_statement:
    FOR '(' declaration ';' condition ';' assignment ')' statement { 
        $$ = create_for_node($3, $5, $7, $9, $1);
    }
    | FOR '(' assignment ';' condition ';' assignment ')' statement { 
        $$ = create_for_node($3, $5, $7, $9, $1);
    }
    | FOR '(' ';' condition ';' assignment ')' statement { 
        $$ = create_for_node(NULL, $4, $6, $8, $1);
    }
    | FOR '(' declaration ';' ';' assignment ')' statement { 
        $$ = create_for_node($3, NULL, $6, $8, $1);
    }
    | FOR '(' assignment ';' ';' assignment ')' statement { 
        $$ = create_for_node($3, NULL, $6, $8, $1);
    }
    | FOR '(' ';' ';' assignment ')' statement { 
        $$ = create_for_node(NULL, NULL, $5, $7, $1);
    }
    | FOR '(' declaration ';' condition ';' ')' statement { 
        $$ = create_for_node($3, $5, NULL, $8, $1);
    }
    | FOR '(' assignment ';' condition ';' ')' statement { 
        $$ = create_for_node($3, $5, NULL, $8, $1);
    }
    | FOR '(' ';' condition ';' ')' statement { 
        $$ = create_for_node(NULL, $4, NULL, $7, $1);
    }
    | FOR '(' ';' ';' ')' statement { 
        $$ = create_for_node(NULL, NULL, NULL, $6, $1);
    }
    ;

compound_statement:
    '{' compound_program '}' { 
        TinyOptProgramNode_t *prog = (TinyOptProgramNode_t*)$2;
        $$ = create_compound_node(prog->statements);
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
        $$ = create_condition_node(OP_EQ, $1, $3);
    }
    | expression NE expression { 
        $$ = create_condition_node(OP_NE, $1, $3);
    }
    | expression '<' expression { 
        $$ = create_condition_node(OP_LT, $1, $3);
    }
    | expression LE expression { 
        $$ = create_condition_node(OP_LE, $1, $3);
    }
    | expression '>' expression { 
        $$ = create_condition_node(OP_GT, $1, $3);
    }
    | expression GE expression { 
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
        $$ = create_identifier_node($1.name, $1.hash);
        free($1.name);
    }
    | ID '[' expression ']' { 
        // Simplificado para demonstração
        $$ = create_identifier_node($1.name, $1.hash);
        free($1.name);
    }
    | function_call { 
        $$ = $1;
    }
    ;

function_call:
    ID '(' ')' {
        $$ = create_function_call_node($1.name, NULL);
        free($1.name);
    }
  | ID '(' argument_list ')' {
        $$ = create_function_call_node($1.name, $3);
        free($1.name);
    }
  ;

argument_list:
    expression {
        $$ = add_args(NULL, $1);
    }
  | argument_list ',' expression {
        $$ = add_args($1, $3);
    }
  ;

function_def:
    INT ID '(' ')' compound_statement {
        $$ = create_function_def_node(TYPE_INT, $2.name, NULL, $5);
        free($2.name);
    }
    | FLOAT ID '(' ')' compound_statement {
        $$ = create_function_def_node(TYPE_FLOAT, $2.name, NULL, $5);
        free($2.name);
    }
    | CHAR ID '(' ')' compound_statement {
        $$ = create_function_def_node(TYPE_CHAR, $2.name, NULL, $5);
        free($2.name);
    }
    | VOID ID '(' ')' compound_statement {
        $$ = create_function_def_node(TYPE_VOID, $2.name, NULL, $5);
        free($2.name);
    }
    | INT ID '(' parameter_list ')' compound_statement {
        $$ = create_function_def_node(TYPE_INT, $2.name, $4, $6);
        free($2.name);
    }
    | FLOAT ID '(' parameter_list ')' compound_statement {
        $$ = create_function_def_node(TYPE_FLOAT, $2.name, $4, $6);
        free($2.name);
    }
    | CHAR ID '(' parameter_list ')' compound_statement {
        $$ = create_function_def_node(TYPE_CHAR, $2.name, $4, $6);
        free($2.name);
    }
    | VOID ID '(' parameter_list ')' compound_statement {
        $$ = create_function_def_node(TYPE_VOID, $2.name, $4, $6);
        free($2.name);
    }
    ;

parameter_list:
    declaration {
        // Criar uma lista com um único parâmetro
        TinyOptProgramNode_t *prog = (TinyOptProgramNode_t*)create_program_node();
        add_statement((TinyOptASTNode_t*)prog, $1);
        $$ = (TinyOptASTNode_t*)prog;
    }
    | parameter_list ',' declaration {
        // Adicionar à lista existente
        TinyOptASTNode_t *param_list = (TinyOptASTNode_t*)$1;
        if (param_list && param_list->type == NODE_PROGRAM) {
            add_statement(param_list, $3);
            $$ = param_list;
        } else {
            // Fallback: criar nova lista
            TinyOptProgramNode_t *prog = (TinyOptProgramNode_t*)create_program_node();
            if (param_list) add_statement((TinyOptASTNode_t*)prog, param_list);
            add_statement((TinyOptASTNode_t*)prog, $3);
            $$ = (TinyOptASTNode_t*)prog;
        }
    }
    | VOID {
        $$ = NULL;
    }
    ;

%%

void yyerror(TinyOptASTNode_t *root, const char *s)
{
    (void)root; // se não usar
    fprintf(stderr, "Erro de sintaxe: %s\n", s);
}