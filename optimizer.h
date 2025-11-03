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
    VariableValue *value;
} VariableInfo;

typedef struct {
    VariableInfo *variables;
    int count;
    int capacity;
} VariableTable;

typedef struct DSETable {
    char *name;
    uint64_t loop_hash;
    struct DSETable *next;
    ASTNode *node;
} DSETable;

void optimize_code(ASTNode *ast);

void remove_dead_code(ASTNode *node);
void set_var_table(ASTNode *node, VariableTable *table, char *current_scope);

void constant_folding(ASTNode *node);
void reachability_analysis(ASTNode *node, VariableTable *table, char *current_scope);
void liveness_and_dead_store_elimination(ASTNode *node, DSETable **table, uint64_t loop_hash);
void empty_blocks(ASTNode *node);

int is_condition_always_true(ASTNode *condition, VariableTable *table, char *current_scope);
int is_condition_always_false(ASTNode *condition, VariableTable *table, char *current_scope);
int has_return_statement(ASTNode *node);
int has_break_continue(ASTNode *node);

VariableTable* create_variable_table();
void add_variable(VariableTable *table, char *scope, char *name);
VariableInfo *find_variable(VariableTable *table, char *scope, char *name);
VariableInfo* set_variable_value(VariableTable *table, char *scope, char *name, VariableValue *value);
VariableValue *create_variable_value_from_node(ASTNode *node, VariableTable *table, char *current_scope);
void free_variable_table(VariableTable *table);
void print_optimization_report(ASTNode *ast);

#endif 