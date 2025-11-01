#include "optimizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_number_node(ASTNode *n);
static long parse_number_value(const char *s);
static ASTNode *make_number(long value);
static ASTNode *fold_expression(ASTNode *node);
static ASTNode *fold_condition(ASTNode *node);
static void fold_in_tree(ASTNode *node);
static int is_pure_expression(ASTNode *expr);
static long eval_relational(Operator op, long a, long b);
static long eval_binary(Operator op, long a, long b);
// static void dse_on_program(ProgramNode *p);
static void add_uses_from_expr(ASTNode *expr, char ***live_names, int *live_count, int *live_cap);
static int live_contains(char **live_names, int live_count, const char *name);
static void live_add(char ***live_names, int *live_count, int *live_cap, const char *name);
void set_parent_recursive(ASTNode *node, ASTNode *parent);

void optimize_code(ASTNode *ast)
{
    set_parent_recursive(ast, NULL);

    VariableTable *var_table = create_variable_table();

    analyze_variable_usage(ast, var_table, "global");

    optimize_unused_variables(ast, var_table, "global");
    optimize_constant_folding(ast);
    optimize_unreachable_code(ast, var_table, "global");
    optimize_redundant_assignments(ast);
    optimize_empty_blocks(ast);

    remove_dead_code(ast);

    // free_variable_table(var_table);
    // var_table = create_variable_table();
    // analyze_variable_usage(ast, var_table, "global");
    // remove_dead_code(ast);

    // print_optimization_report(ast);

    free_variable_table(var_table);
}

void set_parent_recursive(ASTNode *node, ASTNode *parent)
{
    if (!node)
        return;
    node->parent = parent;
    printf("[OPT] Setting parent of node type %d to %d\n", node->type, parent ? parent->type : -1);
    switch (node->type)
    {
    case NODE_PROGRAM:
    {
        ProgramNode *prog = (ProgramNode *)node;
        set_parent_recursive(prog->statements, node);
        break;
    }
    case NODE_DECLARATION:
    {
        DeclarationNode *decl = (DeclarationNode *)node;
        set_parent_recursive(decl->initial_value, node);
        break;
    }
    case NODE_ASSIGNMENT:
    {
        AssignmentNode *assign = (AssignmentNode *)node;
        set_parent_recursive(assign->value, node);
        break;
    }
    case NODE_EXPRESSION:
    {
        ExpressionNode *expr = (ExpressionNode *)node;
        set_parent_recursive(expr->left, node);
        set_parent_recursive(expr->right, node);
        break;
    }
    case NODE_IF_STATEMENT:
    {
        IfNode *ifn = (IfNode *)node;
        set_parent_recursive(ifn->condition, node);
        set_parent_recursive(ifn->then_statement, node);
        set_parent_recursive(ifn->else_statement, node);
        break;
    }
    case NODE_WHILE_STATEMENT:
    {
        WhileNode *w = (WhileNode *)node;
        set_parent_recursive(w->condition, node);
        set_parent_recursive(w->body, node);
        break;
    }
    case NODE_FOR_STATEMENT:
    {
        ForNode *f = (ForNode *)node;
        set_parent_recursive(f->init, node);
        set_parent_recursive(f->condition, node);
        set_parent_recursive(f->increment, node);
        set_parent_recursive(f->body, node);
        break;
    }
    case NODE_COMPOUND_STATEMENT:
    {
        CompoundNode *c = (CompoundNode *)node;
        ASTNode *stmt = c->statements;
        while (stmt)
        {
            set_parent_recursive(stmt, node);
            stmt = stmt->next;
        }
        break;
    }
    case NODE_RETURN:
    {
        ReturnNode *ret = (ReturnNode *)node;
        set_parent_recursive(ret->value, node);
        break;
    }
    case NODE_BINARY_OP:
    {
        BinaryOpNode *bin = (BinaryOpNode *)node;
        set_parent_recursive(bin->left, node);
        set_parent_recursive(bin->right, node);
        break;
    }
    case NODE_UNARY_OP:
    {
        UnaryOpNode *un = (UnaryOpNode *)node;
        set_parent_recursive(un->operand, node);
        break;

        break;
    }
    case NODE_FUNCTION_DEF:
    {
        FunctionDefNode *f = (FunctionDefNode *)node;
        set_parent_recursive(f->parameters, node);
        set_parent_recursive(f->body, node);
        break;
    }
    case NODE_FUNCTION_CALL:
    {
        FunctionCallNode *fc = (FunctionCallNode *)node;
        set_parent_recursive(fc->arguments, node);
        break;
    }
    case NODE_PARAMETER_LIST:
    {
        ParameterListNode *pl = (ParameterListNode *)node;
        set_parent_recursive(pl->parameters, node);
        break;
    }
    default:
        break;
    }

    if (node->next)
        set_parent_recursive(node->next, parent);
}

void analyze_variable_usage(ASTNode *node, VariableTable *table, char *current_scope)
{
    if (node == NULL)
        return;

    switch (node->type)
    {
    case NODE_PROGRAM:
    {
        ProgramNode *prog = (ProgramNode *)node;
        analyze_variable_usage(prog->statements, table, current_scope);
        break;
    }
    case NODE_DECLARATION:
    {
        DeclarationNode *decl = (DeclarationNode *)node;
        if (decl->name)
        {

            add_variable(table, current_scope, decl->name);
            VariableInfo *var = find_variable(table, current_scope, decl->name);
            if (var)
            {
                var->is_defined = 1;
            }

            if (decl->initial_value)
            {
                VariableValue *value = create_variable_value_from_node(decl->initial_value, table, current_scope);
                set_variable_value(table, current_scope, decl->name, value);
            }
        }

        analyze_variable_usage(decl->initial_value, table, current_scope);
        break;
    }
    case NODE_ASSIGNMENT:
    {
        AssignmentNode *assign = (AssignmentNode *)node;
        if (assign->variable)
        {

            add_variable(table, current_scope, assign->variable);
            VariableInfo *var = find_variable(table, current_scope, assign->variable);
            if (var)
            {
                var->is_defined = 1;
            }

            VariableValue *value = create_variable_value_from_node(assign->value, table, current_scope);
            set_variable_value(table, current_scope, assign->variable, value);
        }
        analyze_variable_usage(assign->value, table, current_scope);
        break;
    }
    case NODE_IDENTIFIER:
    {
        IdentifierNode *id = (IdentifierNode *)node;
        if (id->name)
        {
            VariableInfo *var = find_variable(table, current_scope, id->name);
            if (var)
            {
                printf("Variable used: %s in scope %s\n", id->name, current_scope);

                var->is_used = 1;
                // var->node->is_dead_code = 0;
                // printf("%d\n", var->node->is_dead_code);
            }
        }
        break;
    }
    case NODE_EXPRESSION:
    {
        ExpressionNode *expr = (ExpressionNode *)node;
        analyze_variable_usage(expr->left, table, current_scope);
        analyze_variable_usage(expr->right, table, current_scope);
        break;
    }
    case NODE_CONDITION:
    {
        ConditionNode *cond = (ConditionNode *)node;
        analyze_variable_usage(cond->left, table, current_scope);
        analyze_variable_usage(cond->right, table, current_scope);
        break;
    }
    case NODE_IF_STATEMENT:
    {
        IfNode *if_node = (IfNode *)node;
        analyze_variable_usage(if_node->condition, table, current_scope);
        analyze_variable_usage(if_node->then_statement, table, current_scope);
        analyze_variable_usage(if_node->else_statement, table, current_scope);
        break;
    }
    case NODE_WHILE_STATEMENT:
    {
        WhileNode *while_node = (WhileNode *)node;
        analyze_variable_usage(while_node->condition, table, current_scope);
        analyze_variable_usage(while_node->body, table, current_scope);
        break;
    }
    case NODE_FOR_STATEMENT:
    {
        ForNode *for_node = (ForNode *)node;
        analyze_variable_usage(for_node->init, table, current_scope);
        analyze_variable_usage(for_node->condition, table, current_scope);
        analyze_variable_usage(for_node->increment, table, current_scope);
        analyze_variable_usage(for_node->body, table, current_scope);
        break;
    }
    case NODE_COMPOUND_STATEMENT:
    {
        CompoundNode *comp = (CompoundNode *)node;
        if (comp->statements)
        {
            analyze_variable_usage(comp->statements, table, current_scope);
        }
        break;
    }
    case NODE_RETURN:
    {
        ReturnNode *ret = (ReturnNode *)node;
        analyze_variable_usage(ret->value, table, current_scope);
        break;
    }
    case NODE_FUNCTION_DEF:
    {
        FunctionDefNode *func = (FunctionDefNode *)node;
        analyze_variable_usage(func->parameters, table, func->name);
        analyze_variable_usage(func->body, table, func->name);
        break;
    }
    case NODE_FUNCTION_CALL:
    {
        FunctionCallNode *call = (FunctionCallNode *)node;
        analyze_variable_usage(call->arguments, table, current_scope);
        break;
    }
    case NODE_BINARY_OP:
    {
        BinaryOpNode *bin = (BinaryOpNode *)node;
        analyze_variable_usage(bin->left, table, current_scope);
        analyze_variable_usage(bin->right, table, current_scope);
        break;
    }
    case NODE_UNARY_OP:
    {
        UnaryOpNode *unary = (UnaryOpNode *)node;
        analyze_variable_usage(unary->operand, table, current_scope);
        break;
    }
    default:
        break;
    }

    analyze_variable_usage(node->next, table, current_scope);
}

VariableValue *create_variable_value_from_node(ASTNode *node, VariableTable *table, char *current_scope)
{
    if (node == NULL)
        return NULL;

    VariableValue *value = malloc(sizeof(VariableValue));
    value->type = VALUE_TYPE_UNKNOWN;
    value->number = 0;
    value->str = "";

    if (node->type == NODE_NUMBER)
    {
        NumberNode *num = (NumberNode *)node;
        value->type = VALUE_TYPE_INT;
        value->number = parse_number_value(num->value);
    }
    else if (node->type == NODE_STRING_LITERAL)
    {
        StringLiteralNode *str = (StringLiteralNode *)node;
        value->type = VALUE_TYPE_STRING;
        value->str = strdup(str->value);
    }
    else if (node->type == NODE_IDENTIFIER)
    {
        VariableInfo *var = find_variable(table, current_scope, ((IdentifierNode *)node)->name);
        if (var && var->value)
        {
            return var->value;
        }
    }
    // Adicionar suporte para outros tipos conforme necessário

    return value;
}

void optimize_unused_variables(ASTNode *node, VariableTable *table, char *current_scope)
{
    if (node == NULL)
        return;

    switch (node->type)
    {
    case NODE_PROGRAM:
    {
        ProgramNode *prog = (ProgramNode *)node;
        optimize_unused_variables(prog->statements, table, current_scope);
        break;
    }
    case NODE_DECLARATION:
    {
        DeclarationNode *decl = (DeclarationNode *)node;
        if (decl->name)
        {
            VariableInfo *var = find_variable(table, current_scope, decl->name);
            if (var && var->is_defined && !var->is_used)
            {
                node->is_dead_code = 1;
            }
        }
        break;
    }
    case NODE_ASSIGNMENT:
    {
        AssignmentNode *assign = (AssignmentNode *)node;
        if (assign->variable)
        {
            VariableInfo *var = find_variable(table, current_scope, assign->variable);
            if (var && (var->is_defined && !var->is_used))
            {
                node->is_dead_code = 1;
            }
        }
        optimize_unused_variables(assign->value, table, current_scope);
        break;
    }
    case NODE_COMPOUND_STATEMENT:
    {
        CompoundNode *comp = (CompoundNode *)node;
        optimize_unused_variables(comp->statements, table, current_scope);
        break;
    }
    case NODE_IF_STATEMENT:
    {
        IfNode *i = (IfNode *)node;
        optimize_unused_variables(i->then_statement, table, current_scope);
        optimize_unused_variables(i->else_statement, table, current_scope);
        break;
    }
    case NODE_WHILE_STATEMENT:
    {
        WhileNode *w = (WhileNode *)node;
        optimize_unused_variables(w->body, table, current_scope);
        break;
    }
    case NODE_FOR_STATEMENT:
    {
        ForNode *f = (ForNode *)node;
        optimize_unused_variables(f->init, table, current_scope);
        optimize_unused_variables(f->body, table, current_scope);
        optimize_unused_variables(f->increment, table, current_scope);
        break;
    }
    case NODE_FUNCTION_DEF:
    {
        FunctionDefNode *func = (FunctionDefNode *)node;
        optimize_unused_variables(func->body, table, func->name);
        break;
    }
    case NODE_RETURN:
    {
        ReturnNode *ret = (ReturnNode *)node;
        optimize_unused_variables(ret->value, table, current_scope);
        break;
    }
    case NODE_BINARY_OP:
    {
        BinaryOpNode *bin = (BinaryOpNode *)node;
        optimize_unused_variables(bin->left, table, current_scope);
        optimize_unused_variables(bin->right, table, current_scope);
        break;
    }
    case NODE_UNARY_OP:
    {
        UnaryOpNode *unary = (UnaryOpNode *)node;
        optimize_unused_variables(unary->operand, table, current_scope);
        break;
    }
    case NODE_CONDITION:
    {
        ConditionNode *cond = (ConditionNode *)node;
        optimize_unused_variables(cond->left, table, current_scope);
        optimize_unused_variables(cond->right, table, current_scope);
        break;
    }
    default:
        break;
    }

    optimize_unused_variables(node->next, table, current_scope);
}

void optimize_constant_folding(ASTNode *node)
{
    if (node == NULL)
        return;
    fold_in_tree(node);
}

static int is_number_node(ASTNode *n)
{
    return n && n->type == NODE_NUMBER;
}

static long parse_number_value(const char *s)
{
    if (s == NULL)
        return 0;
    return strtol(s, NULL, 10);
}

static ASTNode *make_number(long value)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%ld", value);
    return create_number_node(buf, TYPE_INT);
}

static int is_pure_expression(ASTNode *expr)
{
    if (expr == NULL)
        return 1;
    switch (expr->type)
    {
    case NODE_NUMBER:
    case NODE_IDENTIFIER:
        return 1;
    case NODE_BINARY_OP:
    {
        BinaryOpNode *b = (BinaryOpNode *)expr;
        return is_pure_expression(b->left) && is_pure_expression(b->right);
    }
    case NODE_UNARY_OP:
    {
        UnaryOpNode *u = (UnaryOpNode *)expr;
        return is_pure_expression(u->operand);
    }
    case NODE_CONDITION:
    {
        ConditionNode *c = (ConditionNode *)expr;
        return is_pure_expression(c->left) && is_pure_expression(c->right);
    }
    default:
        return 0;
    }
}

static long eval_relational(Operator op, long a, long b)
{
    switch (op)
    {
    case OP_EQ:
        return a == b;
    case OP_NE:
        return a != b;
    case OP_LT:
        return a < b;
    case OP_LE:
        return a <= b;
    case OP_GT:
        return a > b;
    case OP_GE:
        return a >= b;
    case OP_OR:
        return a || b;
    default:
        return 0;
    }
}

static long eval_binary(Operator op, long a, long b)
{
    switch (op)
    {
    case OP_ADD:
        return a + b;
    case OP_SUB:
        return a - b;
    case OP_MUL:
        return a * b;
    case OP_DIV:
        return b != 0 ? a / b : 0;
    case OP_MOD:
        return b != 0 ? a % b : 0;
    default:
        return 0;
    }
}

static ASTNode *fold_condition(ASTNode *node)
{
    if (node == NULL)
        return NULL;
    switch (node->type)
    {
    case NODE_CONDITION:
    {
        ConditionNode *c = (ConditionNode *)node;
        c->left = fold_expression(c->left);
        c->right = fold_expression(c->right);
        if (is_number_node(c->left) && is_number_node(c->right))
        {
            long a = parse_number_value(((NumberNode *)c->left)->value);
            long b = parse_number_value(((NumberNode *)c->right)->value);
            long res = eval_relational(c->op, a, b);
            return make_number(res);
        }
        return node;
    }
    case NODE_UNARY_OP:
    {
        UnaryOpNode *u = (UnaryOpNode *)node;
        u->operand = fold_condition(u->operand);
        if (u->op == OP_NOT && is_number_node(u->operand))
        {
            long v = parse_number_value(((NumberNode *)u->operand)->value);
            return make_number(!v);
        }
        return node;
    }
    default:
        return fold_expression(node);
    }
}

static ASTNode *fold_expression(ASTNode *node)
{
    if (node == NULL)
        return NULL;
    switch (node->type)
    {
    case NODE_BINARY_OP:
    {
        BinaryOpNode *b = (BinaryOpNode *)node;
        b->left = fold_expression(b->left);
        b->right = fold_expression(b->right);
        if (is_number_node(b->left) && is_number_node(b->right))
        {
            long a = parse_number_value(((NumberNode *)b->left)->value);
            long c = parse_number_value(((NumberNode *)b->right)->value);
            long res = eval_binary(b->op, a, c);
            return make_number(res);
        }
        return node;
    }
    case NODE_UNARY_OP:
    {
        UnaryOpNode *u = (UnaryOpNode *)node;
        u->operand = fold_expression(u->operand);
        if (u->op == OP_NOT && is_number_node(u->operand))
        {
            long v = parse_number_value(((NumberNode *)u->operand)->value);
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

static void fold_in_tree(ASTNode *node)
{
    if (node == NULL)
        return;
    switch (node->type)
    {
    case NODE_PROGRAM:
    {
        ProgramNode *p = (ProgramNode *)node;
        fold_in_tree(p->statements);
        break;
    }
    case NODE_DECLARATION:
    {
        DeclarationNode *d = (DeclarationNode *)node;
        if (d->initial_value)
            d->initial_value = fold_expression(d->initial_value);
        break;
    }
    case NODE_ASSIGNMENT:
    {
        AssignmentNode *a = (AssignmentNode *)node;
        if (a->value)
            a->value = fold_expression(a->value);
        break;
    }
    case NODE_IF_STATEMENT:
    {
        IfNode *i = (IfNode *)node;
        if (i->condition)
            i->condition = fold_condition(i->condition);
        fold_in_tree(i->then_statement);
        fold_in_tree(i->else_statement);
        break;
    }
    case NODE_WHILE_STATEMENT:
    {
        WhileNode *w = (WhileNode *)node;
        if (w->condition)
            w->condition = fold_condition(w->condition);
        fold_in_tree(w->body);
        break;
    }
    case NODE_FOR_STATEMENT:
    {
        ForNode *f = (ForNode *)node;
        if (f->condition)
            f->condition = fold_condition(f->condition);
        fold_in_tree(f->init);
        fold_in_tree(f->increment);
        fold_in_tree(f->body);
        break;
    }
    case NODE_COMPOUND_STATEMENT:
    {
        CompoundNode *c = (CompoundNode *)node;
        fold_in_tree(c->statements);
        break;
    }
    case NODE_RETURN:
    {
        ReturnNode *r = (ReturnNode *)node;
        if (r->value)
            r->value = fold_expression(r->value);
        break;
    }
    case NODE_FUNCTION_DEF:
    {
        FunctionDefNode *func = (FunctionDefNode *)node;
        fold_in_tree(func->body);
        break;
    }
    case NODE_FUNCTION_CALL:
    {
        FunctionCallNode *call = (FunctionCallNode *)node;
        // Fazer fold nos argumentos
        if (call->arguments && call->arguments->type == NODE_PROGRAM)
        {
            ProgramNode *prog = (ProgramNode *)call->arguments;
            ASTNode *cur = prog->statements;
            while (cur)
            {
                ASTNode *next = cur->next;
                if (cur->type != NODE_DECLARATION)
                {
                    ASTNode *folded = fold_expression(cur);
                    if (folded != cur)
                    {
                        // Substituir na lista (simplificado)
                    }
                }
                cur = next;
            }
        }
        break;
    }
    default:
        break;
    }
    fold_in_tree(node->next);
}

void mark_subtree_dead(ASTNode *node)
{
    if (node == NULL)
        return;

    node->is_dead_code = 1;

    switch (node->type)
    {
    case NODE_IF_STATEMENT:
    {
        IfNode *ifn = (IfNode *)node;
        mark_subtree_dead(ifn->condition);
        mark_subtree_dead(ifn->then_statement);
        mark_subtree_dead(ifn->else_statement);
        break;
    }
    case NODE_WHILE_STATEMENT:
    {
        WhileNode *wn = (WhileNode *)node;
        mark_subtree_dead(wn->condition);
        mark_subtree_dead(wn->body);
        break;
    }
    case NODE_FOR_STATEMENT:
    {
        ForNode *fn = (ForNode *)node;
        mark_subtree_dead(fn->init);
        mark_subtree_dead(fn->condition);
        mark_subtree_dead(fn->increment);
        mark_subtree_dead(fn->body);
        break;
    }
    case NODE_COMPOUND_STATEMENT:
    {
        CompoundNode *comp = (CompoundNode *)node;
        ASTNode *stmt = comp->statements;
        while (stmt)
        {
            mark_subtree_dead(stmt);
            stmt = stmt->next;
        }
        break;
    }
    case NODE_FUNCTION_DEF:
    {
        FunctionDefNode *func = (FunctionDefNode *)node;
        mark_subtree_dead(func->body);
        break;
    }
    case NODE_FUNCTION_CALL:
    {
        FunctionCallNode *call = (FunctionCallNode *)node;
        mark_subtree_dead(call->arguments);
        break;
    }
    default:
        break;
    }

    mark_subtree_dead(node->next);
}

void optimize_unreachable_code(ASTNode *node, VariableTable *table, char *current_scope)
{
    if (node == NULL)
        return;

    switch (node->type)
    {
    case NODE_PROGRAM:
    {
        ProgramNode *prog = (ProgramNode *)node;
        optimize_unreachable_code(prog->statements, table, current_scope);
        break;
    }
    case NODE_IF_STATEMENT:
    {
        IfNode *if_node = (IfNode *)node;

        if (is_condition_always_true(if_node->condition, table, current_scope))
        {
            if (if_node->else_statement)
            {
                mark_subtree_dead(if_node->else_statement);
            }
        }
        else if (is_condition_always_false(if_node->condition, table, current_scope))
        {
            if (if_node->then_statement)
            {
                mark_subtree_dead(if_node->then_statement);
            }
            if (!if_node->else_statement)
            {
                node->is_dead_code = 1;
            }
        }

        optimize_unreachable_code(if_node->condition, table, current_scope);
        optimize_unreachable_code(if_node->then_statement, table, current_scope);
        optimize_unreachable_code(if_node->else_statement, table, current_scope);
        break;
    }
    case NODE_WHILE_STATEMENT:
    {
        WhileNode *while_node = (WhileNode *)node;

        if (is_condition_always_false(while_node->condition, table, current_scope))
        {
            if (while_node->body)
            {
                mark_subtree_dead(while_node->body);
            }
            node->is_dead_code = 1;
        }

        optimize_unreachable_code(while_node->condition, table, current_scope);
        optimize_unreachable_code(while_node->body, table, current_scope);
        break;
    }
    case NODE_FOR_STATEMENT:
    {
        ForNode *for_node = (ForNode *)node;

        if (for_node->condition && is_condition_always_false(for_node->condition, table, current_scope))
        {
            if (for_node->body)
            {
                mark_subtree_dead(for_node->body);
            }
            node->is_dead_code = 1;
        }

        optimize_unreachable_code(for_node->init, table, current_scope);
        optimize_unreachable_code(for_node->condition, table, current_scope);
        optimize_unreachable_code(for_node->increment, table, current_scope);
        optimize_unreachable_code(for_node->body, table, current_scope);
        break;
    }
    case NODE_COMPOUND_STATEMENT:
    {
        CompoundNode *comp = (CompoundNode *)node;
        if (comp->statements)
        {
            optimize_unreachable_code(comp->statements, table, current_scope);
        }
        break;
    }
    case NODE_RETURN:
    {
        if (node->next)
        {
            mark_subtree_dead(node->next);
        }
        break;
    }
    case NODE_FUNCTION_DEF:
    {
        FunctionDefNode *func = (FunctionDefNode *)node;
        optimize_unreachable_code(func->body, table, func->name);
        break;
    }
    case NODE_FUNCTION_CALL:
    {
        FunctionCallNode *call = (FunctionCallNode *)node;
        optimize_unreachable_code(call->arguments, table, current_scope);
        break;
    }
    case NODE_BREAK:
    case NODE_CONTINUE:
    {
        if (node->next)
        {
            mark_subtree_dead(node->next);
        }
        break;
    }
    default:
        break;
    }

    optimize_unreachable_code(node->next, table, current_scope);
}

typedef struct UsedVar
{
    char *name;
    struct UsedVar *next;
} UsedVar;

static int var_in_list(UsedVar *list, const char *name)
{
    for (; list; list = list->next)
        if (strcmp(list->name, name) == 0)
            return 1;
    return 0;
}

static void add_var(UsedVar **list, const char *name)
{
    if (var_in_list(*list, name))
        return;
    UsedVar *new_var = malloc(sizeof(UsedVar));
    new_var->name = strdup(name);
    new_var->next = *list;
    *list = new_var;
}

static void free_var_list(UsedVar *list)
{
    while (list)
    {
        UsedVar *tmp = list;
        list = list->next;
        free(tmp->name);
        free(tmp);
    }
}

static ASTNode *get_last_node(ASTNode *head)
{
    if (!head)
        return NULL;
    while (head->next)
        head = head->next;
    return head;
}

static void collect_used_vars(ASTNode *expr, UsedVar **list)
{
    if (!expr)
        return;
    switch (expr->type)
    {
    case NODE_IDENTIFIER:
        add_var(list, ((IdentifierNode *)expr)->name);
        break;
    case NODE_BINARY_OP:
        collect_used_vars(((BinaryOpNode *)expr)->left, list);
        collect_used_vars(((BinaryOpNode *)expr)->right, list);
        break;
    case NODE_UNARY_OP:
        collect_used_vars(((UnaryOpNode *)expr)->operand, list);
        break;
    default:
        break;
    }
}

void optimize_redundant_assignments(ASTNode *node)
{
    ProgramNode *prog = (ProgramNode *) node;
   
    UsedVar *used = NULL;
    ASTNode *current = get_last_node(prog->statements);
  printf("[DSE] Analisando type %d\n", current->type);
    while (current)
    {
       
        if (current->type == NODE_ASSIGNMENT)
        {
            AssignmentNode *assign = (AssignmentNode *)current;
            const char *var_name = assign->variable;

            // Se variável não foi usada ainda → atribuição morta
            if (!var_in_list(used, var_name))
            {
                current->is_dead_code = 1;
                // printf("[DSE] Removendo store morta em '%s' (linha %d)\n", var_name, current->line);
            }

            // Coleta variáveis lidas na expressão da direita
            collect_used_vars(assign->value, &used);

            // Marca variável como usada (a partir daqui pra cima)
            add_var(&used, var_name);
        }

        current = current->parent; // se você encadear via parent
        // ou current = current->prev, se tiver "prev"
        // se não tiver, você pode percorrer do início até o penúltimo para achar o anterior
    }

    free_var_list(used);
}

static int live_contains(char **live_names, int live_count, const char *name)
{
    if (name == NULL)
        return 0;
    for (int i = 0; i < live_count; i++)
    {
        if (live_names[i] && strcmp(live_names[i], name) == 0)
            return 1;
    }
    return 0;
}

static void live_add(char ***live_names, int *live_count, int *live_cap, const char *name)
{
    if (name == NULL)
        return;
    if (live_contains(*live_names, *live_count, name))
        return;
    if (*live_count >= *live_cap)
    {
        *live_cap *= 2;
        *live_names = realloc(*live_names, sizeof(char *) * (*live_cap));
    }
    (*live_names)[(*live_count)++] = strdup(name);
}

static void add_uses_from_expr(ASTNode *expr, char ***live_names, int *live_count, int *live_cap)
{
    if (!expr)
        return;
    switch (expr->type)
    {
    case NODE_IDENTIFIER:
        live_add(live_names, live_count, live_cap, ((IdentifierNode *)expr)->name);
        break;
    case NODE_BINARY_OP:
    {
        BinaryOpNode *b = (BinaryOpNode *)expr;
        add_uses_from_expr(b->left, live_names, live_count, live_cap);
        add_uses_from_expr(b->right, live_names, live_count, live_cap);
        break;
    }
    case NODE_UNARY_OP:
    {
        UnaryOpNode *u = (UnaryOpNode *)expr;
        add_uses_from_expr(u->operand, live_names, live_count, live_cap);
        break;
    }
    case NODE_FUNCTION_CALL:
    {
        FunctionCallNode *call = (FunctionCallNode *)expr;
        // Adicionar usos dos argumentos
        if (call->arguments && call->arguments->type == NODE_PROGRAM)
        {
            ProgramNode *prog = (ProgramNode *)call->arguments;
            ASTNode *cur = prog->statements;
            while (cur)
            {
                add_uses_from_expr(cur, live_names, live_count, live_cap);
                cur = cur->next;
            }
        }
        break;
    }
    case NODE_CONDITION:
    {
        ConditionNode *c = (ConditionNode *)expr;
        add_uses_from_expr(c->left, live_names, live_count, live_cap);
        add_uses_from_expr(c->right, live_names, live_count, live_cap);
        break;
    }
    default:
        break;
    }
}

void optimize_empty_blocks(ASTNode *node)
{
    if (node == NULL)
        return;

    switch (node->type)
    {
    case NODE_PROGRAM:
    {
        ProgramNode *prog = (ProgramNode *)node;
        optimize_empty_blocks(prog->statements);
        break;
    }
    case NODE_COMPOUND_STATEMENT:
    {
        CompoundNode *comp = (CompoundNode *)node;
        if (comp->statements == NULL)
        {
            node->is_dead_code = 1;
        }
        else
        {
            optimize_empty_blocks(comp->statements);
        }
        break;
    }
    case NODE_IF_STATEMENT:
    {
        IfNode *i = (IfNode *)node;
        optimize_empty_blocks(i->then_statement);
        optimize_empty_blocks(i->else_statement);
        break;
    }
    case NODE_WHILE_STATEMENT:
    {
        WhileNode *w = (WhileNode *)node;
        optimize_empty_blocks(w->body);
        break;
    }
    case NODE_FOR_STATEMENT:
    {
        ForNode *f = (ForNode *)node;
        optimize_empty_blocks(f->body);
        break;
    }
    case NODE_FUNCTION_DEF:
    {
        FunctionDefNode *func = (FunctionDefNode *)node;
        optimize_empty_blocks(func->body);
        break;
    }
    default:
        break;
    }

    optimize_empty_blocks(node->next);
}

int is_condition_always_true(ASTNode *condition, VariableTable *table, char *current_scope)
{
    if (condition == NULL)
        return 0;

    switch (condition->type)
    {
    case NODE_NUMBER:
    {
        NumberNode *num = (NumberNode *)condition;
        return strcmp(num->value, "0") != 0;
    }
    case NODE_CONDITION:
    {
        ConditionNode *c = (ConditionNode *)condition;

        long a = 0, b = 0;
        if (c->left)
        {

            if (c->left->type == NODE_NUMBER)
            {
                a = parse_number_value(((NumberNode *)c->left)->value);
            }
            if (c->left->type == NODE_IDENTIFIER)
            {
                IdentifierNode *left_id = (IdentifierNode *)c->left;
                VariableInfo *left_var = find_variable(table, current_scope, left_id->name);
                if (left_var && left_var->is_defined && left_var->value && left_var->value->type == VALUE_TYPE_INT)
                {
                    a = left_var->value->number;
                }
            }
        }

        if (c->right)
        {
            if (c->right->type == NODE_NUMBER)
            {
                b = parse_number_value(((NumberNode *)c->right)->value);
            }
            if (c->right->type == NODE_IDENTIFIER)
            {
                IdentifierNode *right_id = (IdentifierNode *)c->right;
                VariableInfo *right_var = find_variable(table, current_scope, right_id->name);
                if (right_var && right_var->is_defined && right_var->value && right_var->value->type == VALUE_TYPE_INT)
                {
                    b = right_var->value->number;
                }
            }
        }

        return eval_relational(c->op, a, b) != 0;
    }
    case NODE_UNARY_OP:
    {
        UnaryOpNode *u = (UnaryOpNode *)condition;
        if (u->op == OP_NOT && u->operand && u->operand->type == NODE_NUMBER)
        {
            long v = parse_number_value(((NumberNode *)u->operand)->value);
            return (!v) != 0;
        }
        return 0;
    }
    default:
        return 0;
    }
}

int is_condition_always_false(ASTNode *condition, VariableTable *table, char *current_scope)
{
    if (condition == NULL)
        return 0;

    switch (condition->type)
    {
    case NODE_NUMBER:
    {
        NumberNode *num = (NumberNode *)condition;
        return strcmp(num->value, "0") == 0;
    }
    case NODE_CONDITION:
    {
        ConditionNode *c = (ConditionNode *)condition;

        long a = 0, b = 0;
        if (c->left)
        {

            if (c->left->type == NODE_NUMBER)
            {
                a = parse_number_value(((NumberNode *)c->left)->value);
            }
            if (c->left->type == NODE_IDENTIFIER)
            {
                IdentifierNode *left_id = (IdentifierNode *)c->left;
                VariableInfo *left_var = find_variable(table, current_scope, left_id->name);
                if (left_var && left_var->is_defined && left_var->value && left_var->value->type == VALUE_TYPE_INT)
                {
                    a = left_var->value->number;
                }
            }
        }

        if (c->right)
        {
            if (c->right->type == NODE_NUMBER)
            {
                b = parse_number_value(((NumberNode *)c->right)->value);
            }
            if (c->right->type == NODE_IDENTIFIER)
            {
                IdentifierNode *right_id = (IdentifierNode *)c->right;
                VariableInfo *right_var = find_variable(table, current_scope, right_id->name);
                if (right_var && right_var->is_defined && right_var->value && right_var->value->type == VALUE_TYPE_INT)
                {
                    b = right_var->value->number;
                }
            }
        }

        return eval_relational(c->op, a, b) == 0;
    }
    case NODE_UNARY_OP:
    {
        UnaryOpNode *u = (UnaryOpNode *)condition;
        if (u->op == OP_NOT && u->operand && u->operand->type == NODE_NUMBER)
        {
            long v = parse_number_value(((NumberNode *)u->operand)->value);
            return (!v) == 0;
        }
        return 0;
    }
    default:
        return 0;
    }
}

int has_return_statement(ASTNode *node)
{
    if (node == NULL)
        return 0;

    if (node->type == NODE_RETURN)
        return 1;

    return has_return_statement(node->next);
}

int has_break_continue(ASTNode *node)
{
    if (node == NULL)
        return 0;

    if (node->type == NODE_BREAK || node->type == NODE_CONTINUE)
        return 1;

    return has_break_continue(node->next);
}

VariableTable *create_variable_table()
{
    VariableTable *table = malloc(sizeof(VariableTable));
    if (!table)
    {
        return NULL;
    }
    table->capacity = 10;
    table->count = 0;
    table->variables = malloc(sizeof(VariableInfo) * table->capacity);
    if (!table->variables)
    {
        free(table);
        return NULL;
    }
    return table;
}

void add_variable(VariableTable *table, char *scope, char *name)
{
    if (name == NULL || find_variable(table, scope, name) != NULL)
        return;

    if (table->count >= table->capacity)
    {
        table->capacity *= 2;
        table->variables = realloc(table->variables, sizeof(VariableInfo) * table->capacity);
    }

    table->variables[table->count].name = strdup(name);
    table->variables[table->count].scope = strdup(scope);
    table->variables[table->count].is_used = 0;
    table->variables[table->count].is_defined = 0;
    table->variables[table->count].is_dead = 0;
    table->variables[table->count].value = NULL;
    table->count++;
}

VariableInfo *find_variable(VariableTable *table, char *scope, char *name)
{
    if (name == NULL)
        return NULL;

    for (int i = 0; i < table->count; i++)
    {
        if (table->variables[i].name && strcmp(table->variables[i].name, name) == 0 && strcmp(table->variables[i].scope, scope) == 0)
        {
            return &table->variables[i];
        }
    }
    return NULL;
}

VariableInfo *set_variable_value(VariableTable *table, char *scope, char *name, VariableValue *value)
{
    VariableInfo *var = find_variable(table, scope, name);
    if (var && var->is_defined)
    {
        var->value = value;
        return var;
    }
    return NULL;
}

void free_variable_table(VariableTable *table)
{
    for (int i = 0; i < table->count; i++)
    {
        free(table->variables[i].name);
    }
    free(table->variables);
    free(table);
}

void print_optimization_report(ASTNode *ast)
{
    printf("\n=== Relatório de Otimização ===\n");
    printf("Árvore Sintática Abstrata após otimização:\n");
    print_ast(ast, 0);
    printf("\n");
}

static void prune_list(ASTNode **head_ref);
static void prune_children(ASTNode *node);

void remove_dead_code(ASTNode *node)
{
    if (node == NULL)
        return;
    switch (node->type)
    {
    case NODE_PROGRAM:
    {
        ProgramNode *prog = (ProgramNode *)node;
        prune_list(&prog->statements);
        break;
    }
    case NODE_COMPOUND_STATEMENT:
    {
        CompoundNode *comp = (CompoundNode *)node;
        prune_list(&comp->statements);
        break;
    }
    // case NODE_ASSIGNMENT:
    // {
    //     // printf("ENTROU AQUI\n");
    //     // AssignmentNode *an = (AssignmentNode *)node;
    //     // if (an->value)
    //     //     remove_dead_code(an->value);
    //     // break;
    // }
    case NODE_IF_STATEMENT:
    {
        IfNode *ifn = (IfNode *)node;
        prune_children(ifn->then_statement);
        prune_children(ifn->else_statement);
        if (ifn->condition)
            remove_dead_code(ifn->condition);
        if (ifn->then_statement)
            remove_dead_code(ifn->then_statement);
        if (ifn->else_statement)
            remove_dead_code(ifn->else_statement);
        break;
    }
    case NODE_WHILE_STATEMENT:
    {
        WhileNode *wn = (WhileNode *)node;
        prune_children(wn->body);
        if (wn->condition)
            remove_dead_code(wn->condition);
        if (wn->body)
            remove_dead_code(wn->body);
        break;
    }
    case NODE_FOR_STATEMENT:
    {
        ForNode *fn = (ForNode *)node;
        prune_children(fn->init);
        prune_children(fn->increment);
        prune_children(fn->body);
        if (fn->condition)
            remove_dead_code(fn->condition);
        if (fn->init)
            remove_dead_code(fn->init);
        if (fn->increment)
            remove_dead_code(fn->increment);
        if (fn->body)
            remove_dead_code(fn->body);
        break;
    }
    case NODE_FUNCTION_DEF:
    {
        FunctionDefNode *func = (FunctionDefNode *)node;
        if (func->body)
        {
            if (func->body->type == NODE_COMPOUND_STATEMENT)
            {
                CompoundNode *comp = (CompoundNode *)func->body;
                prune_list(&comp->statements);
            }
            remove_dead_code(func->body);
        }
        break;
    }
    default:
        break;
    }
}

static void prune_children(ASTNode *node)
{
    if (node == NULL)
        return;
    if (node->type == NODE_COMPOUND_STATEMENT)
    {
        CompoundNode *comp = (CompoundNode *)node;
        prune_list(&comp->statements);
    }
}

static void prune_list(ASTNode **head_ref)
{
    if (head_ref == NULL)
        return;
    ASTNode *current = *head_ref;
    ASTNode *previous = NULL;
    while (current)
    {
        if (current->is_dead_code)
        {
            ASTNode *to_free = current;
            current = current->next;
            if (previous)
            {
                previous->next = current;
            }
            else
            {
                *head_ref = current;
            }
            to_free->next = NULL;
            free_ast(to_free);
            continue;
        }
        switch (current->type)
        {
        case NODE_IF_STATEMENT:
        {
            IfNode *ifn = (IfNode *)current;
            prune_children(ifn->then_statement);
            prune_children(ifn->else_statement);
            if (ifn->then_statement)
                remove_dead_code(ifn->then_statement);
            if (ifn->else_statement)
                remove_dead_code(ifn->else_statement);
            if (ifn->condition)
                remove_dead_code(ifn->condition);

            if (ifn->then_statement && ifn->then_statement->is_dead_code)
            {
                free_ast(ifn->condition);
                free_ast(ifn->then_statement);
                ifn->then_statement = NULL;
                ifn->condition = NULL;
            }
            if (ifn->else_statement && ifn->else_statement->is_dead_code)
            {
                free_ast(ifn->else_statement);
                ifn->else_statement = NULL;
            }

            break;
        }
        case NODE_WHILE_STATEMENT:
        {
            WhileNode *wn = (WhileNode *)current;
            prune_children(wn->body);
            if (wn->body)
                remove_dead_code(wn->body);
            if (wn->condition)
                remove_dead_code(wn->condition);
            break;
        }
        case NODE_FOR_STATEMENT:
        {
            ForNode *fn = (ForNode *)current;
            prune_children(fn->init);
            prune_children(fn->increment);
            prune_children(fn->body);
            if (fn->init)
                remove_dead_code(fn->init);
            if (fn->increment)
                remove_dead_code(fn->increment);
            if (fn->body)
                remove_dead_code(fn->body);
            if (fn->condition)
                remove_dead_code(fn->condition);
            break;
        }
        case NODE_COMPOUND_STATEMENT:
        {
            CompoundNode *comp = (CompoundNode *)current;
            prune_list(&comp->statements);
            break;
        }
        case NODE_FUNCTION_DEF:
        {
            FunctionDefNode *func = (FunctionDefNode *)current;
            if (func->body)
            {
                if (func->body->type == NODE_COMPOUND_STATEMENT)
                {
                    CompoundNode *comp = (CompoundNode *)func->body;
                    prune_list(&comp->statements);
                }
                remove_dead_code(func->body);
            }
            break;
        }
        default:
            break;
        }
        previous = current;
        current = current->next;
    }
}