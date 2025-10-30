#include "optimizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_number_node(ASTNode *n);
static long parse_number_value(const char *s);
static ASTNode* make_number(long value);
static ASTNode* fold_expression(ASTNode *node);
static ASTNode* fold_condition(ASTNode *node);
static void fold_in_tree(ASTNode *node);
static int is_pure_expression(ASTNode *expr);
static long eval_relational(Operator op, long a, long b);
static long eval_binary(Operator op, long a, long b);
static void dse_on_program(ProgramNode *p);
static void add_uses_from_expr(ASTNode *expr, char ***live_names, int *live_count, int *live_cap);
static int live_contains(char **live_names, int live_count, const char *name);
static void live_add(char ***live_names, int *live_count, int *live_cap, const char *name);

void optimize_code(ASTNode *ast) {
    VariableTable *var_table = create_variable_table();
    
    analyze_variable_usage(ast, var_table);
    
    mark_unused_variables(ast, var_table);
    
    optimize_constant_folding(ast);
    optimize_unreachable_code(ast);
    optimize_redundant_assignments(ast);
    optimize_empty_blocks(ast);
    
    remove_dead_code(ast);

    //print_optimization_report(ast);

    free_variable_table(var_table);
}

void analyze_variable_usage(ASTNode *node, VariableTable *table) {
    if (node == NULL) return;
    
    switch (node->type) {
        case NODE_PROGRAM: {
            ProgramNode *prog = (ProgramNode*)node;
            analyze_variable_usage(prog->statements, table);
            break;
        }
        case NODE_DECLARATION: {
            DeclarationNode *decl = (DeclarationNode*)node;
            if (decl->name) {
                add_variable(table, decl->name);
                VariableInfo *var = find_variable(table, decl->name);
                if (var) {
                    var->is_defined = 1;
                }
            }
            analyze_variable_usage(decl->initial_value, table);
            break;
        }
        case NODE_ASSIGNMENT: {
            AssignmentNode *assign = (AssignmentNode*)node;
            if (assign->variable) {
                add_variable(table, assign->variable);
                VariableInfo *var = find_variable(table, assign->variable);
                if (var) {
                    var->is_defined = 1;
                }
            }
            analyze_variable_usage(assign->value, table);
            break;
        }
        case NODE_IDENTIFIER: {
            IdentifierNode *id = (IdentifierNode*)node;
            if (id->name) {
                VariableInfo *var = find_variable(table, id->name);
                if (var) {
                    var->is_used = 1;
                }
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
            if (comp->statements) {
                analyze_variable_usage(comp->statements, table);
            }
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
    
    analyze_variable_usage(node->next, table);
}

void mark_unused_variables(ASTNode *node, VariableTable *table) {
    if (node == NULL) return;
    
    switch (node->type) {
        case NODE_PROGRAM: {
            ProgramNode *prog = (ProgramNode*)node;
            mark_unused_variables(prog->statements, table);
            break;
        }
        case NODE_DECLARATION: {
            DeclarationNode *decl = (DeclarationNode*)node;
            if (decl->name) {
                VariableInfo *var = find_variable(table, decl->name);
                if (var && var->is_defined && !var->is_used) {
                    node->is_dead_code = 1;
                    // printf("Variável não utilizada marcada como código morto: %s\n", decl->name);
                }
            }
            break;
        }
        case NODE_ASSIGNMENT: {
            AssignmentNode *assign = (AssignmentNode*)node;
            if (assign->variable) {
                VariableInfo *var = find_variable(table, assign->variable);
                if (var && var->is_defined && !var->is_used) {
                    node->is_dead_code = 1;
                    // printf("Atribuição para variável não utilizada marcada como código morto: %s\n", assign->variable);
                }
            }
            break;
        }
        default:
            break;
    }
    
    mark_unused_variables(node->next, table);
}

void optimize_constant_folding(ASTNode *node) {
    if (node == NULL) return;
    fold_in_tree(node);
}

static int is_number_node(ASTNode *n) {
    return n && n->type == NODE_NUMBER;
}

static long parse_number_value(const char *s) {
    if (s == NULL) return 0;
    return strtol(s, NULL, 10);
}

static ASTNode* make_number(long value) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%ld", value);
    return create_number_node(buf, TYPE_INT);
}

static int is_pure_expression(ASTNode *expr) {
    if (expr == NULL) return 1;
    switch (expr->type) {
        case NODE_NUMBER:
        case NODE_IDENTIFIER:
            return 1;
        case NODE_BINARY_OP: {
            BinaryOpNode *b = (BinaryOpNode*)expr;
            return is_pure_expression(b->left) && is_pure_expression(b->right);
        }
        case NODE_UNARY_OP: {
            UnaryOpNode *u = (UnaryOpNode*)expr;
            return is_pure_expression(u->operand);
        }
        case NODE_CONDITION: {
            ConditionNode *c = (ConditionNode*)expr;
            return is_pure_expression(c->left) && is_pure_expression(c->right);
        }
        default:
            return 0;
    }
}

static long eval_relational(Operator op, long a, long b) {
    switch (op) {
        case OP_EQ: return a == b;
        case OP_NE: return a != b;
        case OP_LT: return a < b;
        case OP_LE: return a <= b;
        case OP_GT: return a > b;
        case OP_GE: return a >= b;
        case OP_OR: return a || b;
        default: return 0;
    }
}

static long eval_binary(Operator op, long a, long b) {
    switch (op) {
        case OP_ADD: return a + b;
        case OP_SUB: return a - b;
        case OP_MUL: return a * b;
        case OP_DIV: return b != 0 ? a / b : 0;
        case OP_MOD: return b != 0 ? a % b : 0;
        default: return 0;
    }
}

static ASTNode* fold_condition(ASTNode *node) {
    if (node == NULL) return NULL;
    switch (node->type) {
        case NODE_CONDITION: {
            ConditionNode *c = (ConditionNode*)node;
            c->left = fold_expression(c->left);
            c->right = fold_expression(c->right);
            if (is_number_node(c->left) && is_number_node(c->right)) {
                long a = parse_number_value(((NumberNode*)c->left)->value);
                long b = parse_number_value(((NumberNode*)c->right)->value);
                long res = eval_relational(c->op, a, b);
                return make_number(res);
            }
            return node;
        }
        case NODE_UNARY_OP: {
            UnaryOpNode *u = (UnaryOpNode*)node;
            u->operand = fold_condition(u->operand);
            if (u->op == OP_NOT && is_number_node(u->operand)) {
                long v = parse_number_value(((NumberNode*)u->operand)->value);
                return make_number(!v);
            }
            return node;
        }
        default:
            return fold_expression(node);
    }
}

static ASTNode* fold_expression(ASTNode *node) {
    if (node == NULL) return NULL;
    switch (node->type) {
        case NODE_BINARY_OP: {
            BinaryOpNode *b = (BinaryOpNode*)node;
            b->left = fold_expression(b->left);
            b->right = fold_expression(b->right);
            if (is_number_node(b->left) && is_number_node(b->right)) {
                long a = parse_number_value(((NumberNode*)b->left)->value);
                long c = parse_number_value(((NumberNode*)b->right)->value);
                long res = eval_binary(b->op, a, c);
                return make_number(res);
            }
            return node;
        }
        case NODE_UNARY_OP: {
            UnaryOpNode *u = (UnaryOpNode*)node;
            u->operand = fold_expression(u->operand);
            if (u->op == OP_NOT && is_number_node(u->operand)) {
                long v = parse_number_value(((NumberNode*)u->operand)->value);
                return make_number(!v);
            }
            return node;
        }
        case NODE_CONDITION:
            return fold_condition(node);
        default:
            return node;
    }
}

static void fold_in_tree(ASTNode *node) {
    if (node == NULL) return;
    switch (node->type) {
        case NODE_PROGRAM: {
            ProgramNode *p = (ProgramNode*)node;
            fold_in_tree(p->statements);
            break;
        }
        case NODE_DECLARATION: {
            DeclarationNode *d = (DeclarationNode*)node;
            if (d->initial_value) d->initial_value = fold_expression(d->initial_value);
            break;
        }
        case NODE_ASSIGNMENT: {
            AssignmentNode *a = (AssignmentNode*)node;
            if (a->value) a->value = fold_expression(a->value);
            break;
        }
        case NODE_IF_STATEMENT: {
            IfNode *i = (IfNode*)node;
            if (i->condition) i->condition = fold_condition(i->condition);
            fold_in_tree(i->then_statement);
            fold_in_tree(i->else_statement);
            break;
        }
        case NODE_WHILE_STATEMENT: {
            WhileNode *w = (WhileNode*)node;
            if (w->condition) w->condition = fold_condition(w->condition);
            fold_in_tree(w->body);
            break;
        }
        case NODE_FOR_STATEMENT: {
            ForNode *f = (ForNode*)node;
            if (f->condition) f->condition = fold_condition(f->condition);
            fold_in_tree(f->init);
            fold_in_tree(f->increment);
            fold_in_tree(f->body);
            break;
        }
        case NODE_COMPOUND_STATEMENT: {
            CompoundNode *c = (CompoundNode*)node;
            fold_in_tree(c->statements);
            break;
        }
        default:
            break;
    }
    fold_in_tree(node->next);
}

void mark_subtree_dead(ASTNode *node) {
    if (node == NULL) return;

    node->is_dead_code = 1;

    switch (node->type) {
        case NODE_IF_STATEMENT: {
            IfNode *ifn = (IfNode*)node;
            mark_subtree_dead(ifn->condition);
            mark_subtree_dead(ifn->then_statement);
            mark_subtree_dead(ifn->else_statement);
            break;
        }
        case NODE_WHILE_STATEMENT: {
            WhileNode *wn = (WhileNode*)node;
            mark_subtree_dead(wn->condition);
            mark_subtree_dead(wn->body);
            break;
        }
        case NODE_FOR_STATEMENT: {
            ForNode *fn = (ForNode*)node;
            mark_subtree_dead(fn->init);
            mark_subtree_dead(fn->condition);
            mark_subtree_dead(fn->increment);
            mark_subtree_dead(fn->body);
            break;
        }
        case NODE_COMPOUND_STATEMENT: {
            CompoundNode *comp = (CompoundNode*)node;
            ASTNode *stmt = comp->statements;
            while (stmt) {
                mark_subtree_dead(stmt);
                stmt = stmt->next;
            }
            break;
        }
        default:
            break;
    }

    mark_subtree_dead(node->next);
}

void optimize_unreachable_code(ASTNode *node) {
    if (node == NULL) return;
    
    switch (node->type) {
        case NODE_PROGRAM: {
            ProgramNode *prog = (ProgramNode*)node;
            optimize_unreachable_code(prog->statements);
            break;
        }
        case NODE_IF_STATEMENT: {
            IfNode *if_node = (IfNode*)node;
            
            if (is_condition_always_true(if_node->condition)) {
                if (if_node->else_statement) {
                    mark_subtree_dead(if_node->else_statement);
                    // printf("Else statement marcado como código morto\n");
                }
            }
            else if (is_condition_always_false(if_node->condition)) {
                if (if_node->then_statement) {
                    mark_subtree_dead(if_node->then_statement);
                    // printf("Then statement marcado como código morto\n");
                }
                if (!if_node->else_statement) {
                    node->is_dead_code = 1;
                    // printf("If statement com condição sempre falsa marcado como código morto\n");
                }
            }

            optimize_unreachable_code(if_node->condition);
            optimize_unreachable_code(if_node->then_statement);
            optimize_unreachable_code(if_node->else_statement);
            break;
        }
        case NODE_WHILE_STATEMENT: {
            WhileNode *while_node = (WhileNode*)node;
            
            if (is_condition_always_false(while_node->condition)) {
                if (while_node->body) {
                    mark_subtree_dead(while_node->body);
                    // printf("Corpo do while marcado como código morto\n");
                }
                node->is_dead_code = 1;
                // printf("While statement com condição sempre falsa marcado como código morto\n");
            }
            
            optimize_unreachable_code(while_node->condition);
            optimize_unreachable_code(while_node->body);
            break;
        }
        case NODE_FOR_STATEMENT: {
            ForNode *for_node = (ForNode*)node;

            if (for_node->condition && is_condition_always_false(for_node->condition)) {
                if (for_node->body) {
                    mark_subtree_dead(for_node->body);
                    // printf("Corpo do for marcado como código morto\n");
                }
                node->is_dead_code = 1;
                // printf("For statement com condição sempre falsa marcado como código morto\n");
            }
            
            optimize_unreachable_code(for_node->init);
            optimize_unreachable_code(for_node->condition);
            optimize_unreachable_code(for_node->increment);
            optimize_unreachable_code(for_node->body);
            break;
        }
        case NODE_COMPOUND_STATEMENT: {
            CompoundNode *comp = (CompoundNode*)node;
            if (comp->statements) {
                optimize_unreachable_code(comp->statements);
            }
            break;
        }
        case NODE_RETURN: {
            if (node->next) {
                mark_subtree_dead(node->next);
                // printf("Código após return marcado como morto\n");
            }
            break;
        }
        case NODE_BREAK:
        case NODE_CONTINUE: {
            if (node->next) {
                mark_subtree_dead(node->next);
                // printf("Código após break/continue marcado como morto\n");
            }
            break;
        }
        default:
            break;
    }
    
    optimize_unreachable_code(node->next);
}

void optimize_redundant_assignments(ASTNode *node) {
    if (node == NULL) return;
    
    switch (node->type) {
        case NODE_PROGRAM: {
            ProgramNode *p = (ProgramNode*)node;
            dse_on_program(p);
            break;
        }
        case NODE_COMPOUND_STATEMENT: {
            CompoundNode *c = (CompoundNode*)node;
            if (c->statements) optimize_redundant_assignments(c->statements);
            break;
        }
        case NODE_IF_STATEMENT: {
            IfNode *i = (IfNode*)node;
            if (i->then_statement) optimize_redundant_assignments(i->then_statement);
            if (i->else_statement) optimize_redundant_assignments(i->else_statement);
            break;
        }
        case NODE_WHILE_STATEMENT: {
            WhileNode *w = (WhileNode*)node;
            if (w->body) optimize_redundant_assignments(w->body);
            break;
        }
        case NODE_FOR_STATEMENT: {
            ForNode *f = (ForNode*)node;
            if (f->init) optimize_redundant_assignments(f->init);
            if (f->increment) optimize_redundant_assignments(f->increment);
            if (f->body) optimize_redundant_assignments(f->body);
            break;
        }
        default:
            break;
    }
    
    optimize_redundant_assignments(node->next);
}

static int live_contains(char **live_names, int live_count, const char *name) {
    if (name == NULL) return 0;
    for (int i = 0; i < live_count; i++) {
        if (live_names[i] && strcmp(live_names[i], name) == 0) return 1;
    }
    return 0;
}

static void live_add(char ***live_names, int *live_count, int *live_cap, const char *name) {
    if (name == NULL) return;
    if (live_contains(*live_names, *live_count, name)) return;
    if (*live_count >= *live_cap) {
        *live_cap *= 2;
        *live_names = realloc(*live_names, sizeof(char*) * (*live_cap));
    }
    (*live_names)[(*live_count)++] = strdup(name);
}

static void add_uses_from_expr(ASTNode *expr, char ***live_names, int *live_count, int *live_cap) {
    if (!expr) return;
    switch (expr->type) {
        case NODE_IDENTIFIER:
            live_add(live_names, live_count, live_cap, ((IdentifierNode*)expr)->name);
            break;
        case NODE_BINARY_OP: {
            BinaryOpNode *b = (BinaryOpNode*)expr;
            add_uses_from_expr(b->left, live_names, live_count, live_cap);
            add_uses_from_expr(b->right, live_names, live_count, live_cap);
            break;
        }
        case NODE_UNARY_OP: {
            UnaryOpNode *u = (UnaryOpNode*)expr;
            add_uses_from_expr(u->operand, live_names, live_count, live_cap);
            break;
        }
        case NODE_CONDITION: {
            ConditionNode *c = (ConditionNode*)expr;
            add_uses_from_expr(c->left, live_names, live_count, live_cap);
            add_uses_from_expr(c->right, live_names, live_count, live_cap);
            break;
        }
        default:
            break;
    }
}

static void dse_on_program(ProgramNode *p) {
    if (!p) return;
    int cap = 32, count = 0;
    ASTNode **arr = malloc(sizeof(ASTNode*) * cap);
    for (ASTNode *cur = p->statements; cur; cur = cur->next) {
        if (count >= cap) { cap *= 2; arr = realloc(arr, sizeof(ASTNode*) * cap); }
        arr[count++] = cur;
    }
    int live_cap = 32, live_count = 0;
    char **live = malloc(sizeof(char*) * live_cap);
    for (int i = count - 1; i >= 0; i--) {
        ASTNode *stmt = arr[i];
        switch (stmt->type) {
            case NODE_ASSIGNMENT: {
                AssignmentNode *as = (AssignmentNode*)stmt;
                if (as->op == OP_ADD_ASSIGN || as->op == OP_SUB_ASSIGN ||
                    as->op == OP_MUL_ASSIGN || as->op == OP_DIV_ASSIGN ||
                    as->op == OP_MOD_ASSIGN || as->op == OP_INC || as->op == OP_DEC) {
                    live_add(&live, &live_count, &live_cap, as->variable);
                }
                add_uses_from_expr(as->value, &live, &live_count, &live_cap);
                if (!live_contains(live, live_count, as->variable)) {
                    if (is_pure_expression(as->value)) {
                        stmt->is_dead_code = 1;
                        break;
                    }
                }
                live_add(&live, &live_count, &live_cap, as->variable);
                break;
            }
            case NODE_DECLARATION: {
                DeclarationNode *d = (DeclarationNode*)stmt;
                if (d->initial_value) {
                    add_uses_from_expr(d->initial_value, &live, &live_count, &live_cap);
                    if (!live_contains(live, live_count, d->name) && is_pure_expression(d->initial_value)) {
                        stmt->is_dead_code = 1;
                        break;
                    }
                }
                break;
            }
            case NODE_RETURN: {
                ReturnNode *r = (ReturnNode*)stmt;
                add_uses_from_expr(r->value, &live, &live_count, &live_cap);
                for (int k = 0; k < live_count; k++) { free(live[k]); }
                live_count = 0;
                break;
            }
            case NODE_IF_STATEMENT: {
                IfNode *in = (IfNode*)stmt;
                add_uses_from_expr(in->condition, &live, &live_count, &live_cap);
                break;
            }
            case NODE_WHILE_STATEMENT: {
                WhileNode *wn = (WhileNode*)stmt;
                add_uses_from_expr(wn->condition, &live, &live_count, &live_cap);
                break;
            }
            case NODE_FOR_STATEMENT: {
                ForNode *fn = (ForNode*)stmt;
                add_uses_from_expr(fn->condition, &live, &live_count, &live_cap);
                break;
            }
            default:
                break;
        }
    }
    for (int i = 0; i < live_count; i++) free(live[i]);
    free(live);
    free(arr);
}

void optimize_empty_blocks(ASTNode *node) {
    if (node == NULL) return;
    
    switch (node->type) {
        case NODE_PROGRAM: {
            ProgramNode *prog = (ProgramNode*)node;
            optimize_empty_blocks(prog->statements);
            break;
        }
        case NODE_COMPOUND_STATEMENT: {
            CompoundNode *comp = (CompoundNode*)node;
            if (comp->statements == NULL) {
                node->is_dead_code = 1;
                // printf("Bloco vazio marcado como código morto\n");
            }
            break;
        }
        default:
            break;
    }
    
    optimize_empty_blocks(node->next);
}

int is_condition_always_true(ASTNode *condition) {
    if (condition == NULL) return 0;
    
    switch (condition->type) {
        case NODE_NUMBER: {
            NumberNode *num = (NumberNode*)condition;
            return strcmp(num->value, "0") != 0;
        }
        case NODE_CONDITION: {
            ConditionNode *c = (ConditionNode*)condition;
            if (c->left && c->right && c->left->type == NODE_NUMBER && c->right->type == NODE_NUMBER) {
                long a = parse_number_value(((NumberNode*)c->left)->value);
                long b = parse_number_value(((NumberNode*)c->right)->value);
                return eval_relational(c->op, a, b) != 0;
            }
            return 0;
        }
        case NODE_UNARY_OP: {
            UnaryOpNode *u = (UnaryOpNode*)condition;
            if (u->op == OP_NOT && u->operand && u->operand->type == NODE_NUMBER) {
                long v = parse_number_value(((NumberNode*)u->operand)->value);
                return (!v) != 0;
            }
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
            ConditionNode *c = (ConditionNode*)condition;
            if (c->left && c->right && c->left->type == NODE_NUMBER && c->right->type == NODE_NUMBER) {
                long a = parse_number_value(((NumberNode*)c->left)->value);
                long b = parse_number_value(((NumberNode*)c->right)->value);
                return eval_relational(c->op, a, b) == 0;
            }
            return 0;
        }
        case NODE_UNARY_OP: {
            UnaryOpNode *u = (UnaryOpNode*)condition;
            if (u->op == OP_NOT && u->operand && u->operand->type == NODE_NUMBER) {
                long v = parse_number_value(((NumberNode*)u->operand)->value);
                return (!v) == 0;
            }
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

VariableTable* create_variable_table() {
    VariableTable *table = malloc(sizeof(VariableTable));
    if (!table) {
        return NULL;
    }
    table->capacity = 10;
    table->count = 0;
    table->variables = malloc(sizeof(VariableInfo) * table->capacity);
    if (!table->variables) {
        free(table);
        return NULL;
    }
    return table;
}

void add_variable(VariableTable *table, char *name) {
    if (name == NULL || find_variable(table, name) != NULL) return;
    
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
    if (name == NULL) return NULL;
    
    for (int i = 0; i < table->count; i++) {
        if (table->variables[i].variable && strcmp(table->variables[i].variable, name) == 0) {
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

static void prune_list(ASTNode **head_ref);
static void prune_children(ASTNode *node);

void remove_dead_code(ASTNode *node) {
    if (node == NULL) return;
    switch (node->type) {
        case NODE_PROGRAM: {
            ProgramNode *prog = (ProgramNode*)node;
            prune_list(&prog->statements);
            break;
        }
        case NODE_COMPOUND_STATEMENT: {
            CompoundNode *comp = (CompoundNode*)node;
            prune_list(&comp->statements);
            break;
        }
        case NODE_IF_STATEMENT: {
            IfNode *ifn = (IfNode*)node;
            prune_children(ifn->then_statement);
            prune_children(ifn->else_statement);
            if (ifn->condition) remove_dead_code(ifn->condition);
            if (ifn->then_statement) remove_dead_code(ifn->then_statement);
            if (ifn->else_statement) remove_dead_code(ifn->else_statement);
            break;
        }
        case NODE_WHILE_STATEMENT: {
            WhileNode *wn = (WhileNode*)node;
            prune_children(wn->body);
            if (wn->condition) remove_dead_code(wn->condition);
            if (wn->body) remove_dead_code(wn->body);
            break;
        }
        case NODE_FOR_STATEMENT: {
            ForNode *fn = (ForNode*)node;
            prune_children(fn->init);
            prune_children(fn->increment);
            prune_children(fn->body);
            if (fn->condition) remove_dead_code(fn->condition);
            if (fn->init) remove_dead_code(fn->init);
            if (fn->increment) remove_dead_code(fn->increment);
            if (fn->body) remove_dead_code(fn->body);
            break;
        }
        default:
            break;
    }
}

static void prune_children(ASTNode *node) {
    if (node == NULL) return;
    if (node->type == NODE_COMPOUND_STATEMENT) {
        CompoundNode *comp = (CompoundNode*)node;
        prune_list(&comp->statements);
    }
}

static void prune_list(ASTNode **head_ref) {
    if (head_ref == NULL) return;
    ASTNode *current = *head_ref;
    ASTNode *previous = NULL;
    while (current) {
        if (current->is_dead_code) {
            ASTNode *to_free = current;
            current = current->next;
            if (previous) {
                previous->next = current;
            } else {
                *head_ref = current;
            }
            to_free->next = NULL;
            free_ast(to_free);
            continue;
        }
        switch (current->type) {
            case NODE_IF_STATEMENT: {
                IfNode *ifn = (IfNode*)current;
                prune_children(ifn->then_statement);
                prune_children(ifn->else_statement);
                if (ifn->then_statement) remove_dead_code(ifn->then_statement);
                if (ifn->else_statement) remove_dead_code(ifn->else_statement);
                if (ifn->condition) remove_dead_code(ifn->condition);

                if (ifn->then_statement && ifn->then_statement->is_dead_code) {
                    free_ast(ifn->condition);
                    free_ast(ifn->then_statement);
                    ifn->then_statement = NULL;
                    ifn->condition = NULL;
                }
                if (ifn->else_statement && ifn->else_statement->is_dead_code) {
                    free_ast(ifn->else_statement);
                    ifn->else_statement = NULL;
                }
        
                break;
            }
            case NODE_WHILE_STATEMENT: {
                WhileNode *wn = (WhileNode*)current;
                prune_children(wn->body);
                if (wn->body) remove_dead_code(wn->body);
                if (wn->condition) remove_dead_code(wn->condition);
                break;
            }
            case NODE_FOR_STATEMENT: {
                ForNode *fn = (ForNode*)current;
                prune_children(fn->init);
                prune_children(fn->increment);
                prune_children(fn->body);
                if (fn->init) remove_dead_code(fn->init);
                if (fn->increment) remove_dead_code(fn->increment);
                if (fn->body) remove_dead_code(fn->body);
                if (fn->condition) remove_dead_code(fn->condition);
                break;
            }
            case NODE_COMPOUND_STATEMENT: {
                CompoundNode *comp = (CompoundNode*)current;
                prune_list(&comp->statements);
                break;
            }
            default:
                break;
        }
        previous = current;
        current = current->next;
    }
}