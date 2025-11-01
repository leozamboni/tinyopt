#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "ast.h"

typedef enum {
    VALUE_TYPE_INT,
    VALUE_TYPE_STRING,
    VALUE_TYPE_UNKNOWN
} ValueType;

typedef struct {
    char *str;
    int number;
    ValueType type;
} VariableValue;

typedef struct {
    char *name;
    char *scope;
    char *string_value;
    int is_used;
    int is_defined;
    int is_dead;
    ASTNode *node;
    VariableValue *value;
} VariableInfo;

typedef struct {
    VariableInfo *variables;
    int count;
    int capacity;
} VariableTable;

void optimize_code(ASTNode *ast);

void mark_dead_code(ASTNode *node);
void remove_dead_code(ASTNode *node);
void analyze_variable_usage(ASTNode *node, VariableTable *table, char *current_scope);

void optimize_unused_variables(ASTNode *node, VariableTable *table, char *current_scope);
void optimize_constant_folding(ASTNode *node);
void optimize_unreachable_code(ASTNode *node, VariableTable *table, char *current_scope);
void optimize_redundant_assignments(ASTNode *node, VariableTable *table, char *current_scope);
void optimize_empty_blocks(ASTNode *node);

int is_condition_always_true(ASTNode *condition, VariableTable *table, char *current_scope);
int is_condition_always_false(ASTNode *condition, VariableTable *table, char *current_scope);
int has_return_statement(ASTNode *node);
int has_break_continue(ASTNode *node);

VariableTable* create_variable_table();
void add_variable(VariableTable *table, char *scope, char *name, ASTNode *node);
VariableInfo *find_variable(VariableTable *table, char *scope, char *name);
VariableInfo* set_variable_value(VariableTable *table, char *scope, char *name, VariableValue *value);
VariableValue *create_variable_value_from_node(ASTNode *node, VariableTable *table, char *current_scope);
void free_variable_table(VariableTable *table);
void print_optimization_report(ASTNode *ast);

#endif 