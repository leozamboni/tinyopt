#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "ast.h"

// Estrutura para análise de fluxo de dados
typedef struct {
    char *variable;
    int is_used;
    int is_defined;
    int is_dead;
} VariableInfo;

typedef struct {
    VariableInfo *variables;
    int count;
    int capacity;
} VariableTable;

// Funções de otimização
void optimize_code(ASTNode *ast);

// Análise de código morto
void mark_dead_code(ASTNode *node);
void remove_dead_code(ASTNode *node);
void analyze_variable_usage(ASTNode *node, VariableTable *table);
void mark_unused_variables(ASTNode *node, VariableTable *table);

// Otimizações específicas
void optimize_constant_folding(ASTNode *node);
void optimize_unreachable_code(ASTNode *node);
void optimize_redundant_assignments(ASTNode *node);
void optimize_empty_blocks(ASTNode *node);

// Análise de fluxo de controle
int is_condition_always_true(ASTNode *condition);
int is_condition_always_false(ASTNode *condition);
int has_return_statement(ASTNode *node);
int has_break_continue(ASTNode *node);

// Funções auxiliares
VariableTable* create_variable_table();
void add_variable(VariableTable *table, char *name);
VariableInfo* find_variable(VariableTable *table, char *name);
void free_variable_table(VariableTable *table);
void print_optimization_report(ASTNode *ast);

#endif // OPTIMIZER_H 