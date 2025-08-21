#include "optimizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Função principal de otimização
void optimize_code(ASTNode *ast) {
    printf("=== Iniciando Otimização de Código ===\n");
    
    // Criar tabela de variáveis
    VariableTable *var_table = create_variable_table();
    
    // Análise de uso de variáveis
    analyze_variable_usage(ast, var_table);
    
    // Marcar código morto
    mark_dead_code(ast);
    
    // Marcar variáveis não utilizadas
    mark_unused_variables(ast, var_table);
    
    // Otimizações específicas
    optimize_constant_folding(ast);
    optimize_unreachable_code(ast);
    optimize_redundant_assignments(ast);
    optimize_empty_blocks(ast);
    
    // Relatório de otimização
    print_optimization_report(ast);
    
    // Limpar tabela
    free_variable_table(var_table);
    
    printf("=== Otimização Concluída ===\n");
}

// Análise de uso de variáveis
void analyze_variable_usage(ASTNode *node, VariableTable *table) {
    if (node == NULL) return;
    
    switch (node->type) {
        case NODE_DECLARATION: {
            DeclarationNode *decl = (DeclarationNode*)node;
            add_variable(table, decl->name);
            VariableInfo *var = find_variable(table, decl->name);
            if (var) {
                var->is_defined = 1;
            }
            analyze_variable_usage(decl->initial_value, table);
            break;
        }
        case NODE_ASSIGNMENT: {
            AssignmentNode *assign = (AssignmentNode*)node;
            add_variable(table, assign->variable);
            VariableInfo *var = find_variable(table, assign->variable);
            if (var) {
                var->is_defined = 1;
            }
            analyze_variable_usage(assign->value, table);
            break;
        }
        case NODE_IDENTIFIER: {
            IdentifierNode *id = (IdentifierNode*)node;
            VariableInfo *var = find_variable(table, id->name);
            if (var) {
                var->is_used = 1;
            }
            break;
        }
        case NODE_EXPRESSION: {
            ExpressionNode *expr = (ExpressionNode*)node;
            analyze_variable_usage(expr->left, table);
            analyze_variable_usage(expr->right, table);
            break;
        }
        case NODE_CONDITION: {
            ConditionNode *cond = (ConditionNode*)node;
            analyze_variable_usage(cond->left, table);
            analyze_variable_usage(cond->right, table);
            break;
        }
        case NODE_IF_STATEMENT: {
            IfNode *if_node = (IfNode*)node;
            analyze_variable_usage(if_node->condition, table);
            analyze_variable_usage(if_node->then_statement, table);
            analyze_variable_usage(if_node->else_statement, table);
            break;
        }
        case NODE_WHILE_STATEMENT: {
            WhileNode *while_node = (WhileNode*)node;
            analyze_variable_usage(while_node->condition, table);
            analyze_variable_usage(while_node->body, table);
            break;
        }
        case NODE_FOR_STATEMENT: {
            ForNode *for_node = (ForNode*)node;
            analyze_variable_usage(for_node->init, table);
            analyze_variable_usage(for_node->condition, table);
            analyze_variable_usage(for_node->increment, table);
            analyze_variable_usage(for_node->body, table);
            break;
        }
        case NODE_COMPOUND_STATEMENT: {
            CompoundNode *comp = (CompoundNode*)node;
            analyze_variable_usage(comp->statements, table);
            break;
        }
        case NODE_RETURN: {
            ReturnNode *ret = (ReturnNode*)node;
            analyze_variable_usage(ret->value, table);
            break;
        }
        default:
            break;
    }
    
    // Processar próximo nó na lista
    analyze_variable_usage(node->next, table);
}

// Marcar código morto
void mark_dead_code(ASTNode *node) {
    if (node == NULL) return;
    
    switch (node->type) {
        case NODE_IF_STATEMENT: {
            IfNode *if_node = (IfNode*)node;
            
            // Verificar se a condição é sempre verdadeira ou falsa
            if (is_condition_always_true(if_node->condition)) {
                // Marcar else como código morto
                if (if_node->else_statement) {
                    if_node->else_statement->is_dead_code = 1;
                    mark_dead_code(if_node->else_statement);
                }
            } else if (is_condition_always_false(if_node->condition)) {
                // Marcar then como código morto
                if_node->then_statement->is_dead_code = 1;
                mark_dead_code(if_node->then_statement);
            }
            
            mark_dead_code(if_node->condition);
            mark_dead_code(if_node->then_statement);
            mark_dead_code(if_node->else_statement);
            break;
        }
        case NODE_WHILE_STATEMENT: {
            WhileNode *while_node = (WhileNode*)node;
            
            // Se a condição é sempre falsa, marcar o corpo como código morto
            if (is_condition_always_false(while_node->condition)) {
                while_node->body->is_dead_code = 1;
                mark_dead_code(while_node->body);
            }
            
            mark_dead_code(while_node->condition);
            mark_dead_code(while_node->body);
            break;
        }
        case NODE_FOR_STATEMENT: {
            ForNode *for_node = (ForNode*)node;
            
            // Se a condição é sempre falsa, marcar o corpo como código morto
            if (for_node->condition && is_condition_always_false(for_node->condition)) {
                for_node->body->is_dead_code = 1;
                mark_dead_code(for_node->body);
            }
            
            mark_dead_code(for_node->init);
            mark_dead_code(for_node->condition);
            mark_dead_code(for_node->increment);
            mark_dead_code(for_node->body);
            break;
        }
        case NODE_COMPOUND_STATEMENT: {
            CompoundNode *comp = (CompoundNode*)node;
            mark_dead_code(comp->statements);
            break;
        }
        case NODE_RETURN: {
            // Código após return é inalcançável
            if (node->next) {
                node->next->is_dead_code = 1;
                mark_dead_code(node->next);
            }
            break;
        }
        case NODE_BREAK:
        case NODE_CONTINUE: {
            // Código após break/continue é inalcançável
            if (node->next) {
                node->next->is_dead_code = 1;
                mark_dead_code(node->next);
            }
            break;
        }
        default:
            break;
    }
    
    // Processar próximo nó na lista
    mark_dead_code(node->next);
}

// Marcar variáveis não utilizadas
void mark_unused_variables(ASTNode *node, VariableTable *table) {
    if (node == NULL) return;
    
    switch (node->type) {
        case NODE_DECLARATION: {
            DeclarationNode *decl = (DeclarationNode*)node;
            VariableInfo *var = find_variable(table, decl->name);
            if (var && var->is_defined && !var->is_used) {
                node->is_dead_code = 1;
                printf("Variável não utilizada marcada como código morto: %s\n", decl->name);
            }
            break;
        }
        case NODE_ASSIGNMENT: {
            AssignmentNode *assign = (AssignmentNode*)node;
            VariableInfo *var = find_variable(table, assign->variable);
            if (var && var->is_defined && !var->is_used) {
                node->is_dead_code = 1;
                printf("Atribuição para variável não utilizada marcada como código morto: %s\n", assign->variable);
            }
            break;
        }
        default:
            break;
    }
    
    // Processar próximo nó na lista
    mark_unused_variables(node->next, table);
}

// Otimizações específicas
void optimize_constant_folding(ASTNode *node) {
    if (node == NULL) return;
    
    // Implementar dobramento de constantes
    // Por exemplo: 2 + 3 -> 5
    
    optimize_constant_folding(node->next);
}

void optimize_unreachable_code(ASTNode *node) {
    if (node == NULL) return;
    
    // Código inalcançável já é marcado em mark_dead_code()
    
    optimize_unreachable_code(node->next);
}

void optimize_redundant_assignments(ASTNode *node) {
    if (node == NULL) return;
    
    // Identificar atribuições redundantes
    // Por exemplo: x = 5; x = 10; (primeira atribuição é redundante)
    
    optimize_redundant_assignments(node->next);
}

void optimize_empty_blocks(ASTNode *node) {
    if (node == NULL) return;
    
    switch (node->type) {
        case NODE_COMPOUND_STATEMENT: {
            CompoundNode *comp = (CompoundNode*)node;
            if (comp->statements == NULL) {
                node->is_dead_code = 1;
                printf("Bloco vazio marcado como código morto\n");
            }
            break;
        }
        default:
            break;
    }
    
    optimize_empty_blocks(node->next);
}

// Análise de fluxo de controle
int is_condition_always_true(ASTNode *condition) {
    if (condition == NULL) return 0;
    
    switch (condition->type) {
        case NODE_NUMBER: {
            NumberNode *num = (NumberNode*)condition;
            return strcmp(num->value, "0") != 0;
        }
        case NODE_CONDITION: {
            // Análise mais complexa de condições
            return 0;
        }
        default:
            return 0;
    }
}

int is_condition_always_false(ASTNode *condition) {
    if (condition == NULL) return 0;
    
    switch (condition->type) {
        case NODE_NUMBER: {
            NumberNode *num = (NumberNode*)condition;
            return strcmp(num->value, "0") == 0;
        }
        case NODE_CONDITION: {
            // Análise mais complexa de condições
            return 0;
        }
        default:
            return 0;
    }
}

int has_return_statement(ASTNode *node) {
    if (node == NULL) return 0;
    
    if (node->type == NODE_RETURN) return 1;
    
    return has_return_statement(node->next);
}

int has_break_continue(ASTNode *node) {
    if (node == NULL) return 0;
    
    if (node->type == NODE_BREAK || node->type == NODE_CONTINUE) return 1;
    
    return has_break_continue(node->next);
}

// Funções auxiliares
VariableTable* create_variable_table() {
    VariableTable *table = malloc(sizeof(VariableTable));
    table->capacity = 10;
    table->count = 0;
    table->variables = malloc(sizeof(VariableInfo) * table->capacity);
    return table;
}

void add_variable(VariableTable *table, char *name) {
    if (find_variable(table, name) != NULL) return;
    
    if (table->count >= table->capacity) {
        table->capacity *= 2;
        table->variables = realloc(table->variables, sizeof(VariableInfo) * table->capacity);
    }
    
    table->variables[table->count].variable = strdup(name);
    table->variables[table->count].is_used = 0;
    table->variables[table->count].is_defined = 0;
    table->variables[table->count].is_dead = 0;
    table->count++;
}

VariableInfo* find_variable(VariableTable *table, char *name) {
    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->variables[i].variable, name) == 0) {
            return &table->variables[i];
        }
    }
    return NULL;
}

void free_variable_table(VariableTable *table) {
    for (int i = 0; i < table->count; i++) {
        free(table->variables[i].variable);
    }
    free(table->variables);
    free(table);
}

void print_optimization_report(ASTNode *ast) {
    printf("\n=== Relatório de Otimização ===\n");
    printf("Árvore Sintática Abstrata após otimização:\n");
    print_ast(ast, 0);
    printf("\n");
} 