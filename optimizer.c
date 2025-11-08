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

void optimize_code(ASTNode *ast)
{
    VariableTable *var_table = create_variable_table();

    set_var_table(ast, var_table, "global");

    constant_folding(ast);
    reachability_analysis(ast, var_table, "global");
    empty_blocks(ast);
    liveness_and_dead_store_elimination(ast, NULL, 0);

    remove_dead_code(ast);

    // print_optimization_report(ast);

    free_variable_table(var_table);
}

void set_var_table(ASTNode *node, VariableTable *table, char *current_scope)
{
    if (node == NULL)
        return;

    switch (node->type)
    {
    case NODE_PROGRAM:
    {
        ProgramNode *prog = (ProgramNode *)node;
        set_var_table(prog->statements, table, current_scope);
        break;
    }
    case NODE_DECLARATION:
    {
        DeclarationNode *decl = (DeclarationNode *)node;
        if (decl->name)
        {
            add_variable(table, current_scope, decl->name);
            if (decl->initial_value)
            {
                VariableValue *value = create_variable_value_from_node(decl->initial_value, table, current_scope);
                set_variable_value(table, current_scope, decl->name, value);
            }
        }
        set_var_table(decl->initial_value, table, current_scope);
        break;
    }
    case NODE_ASSIGNMENT:
    {
        AssignmentNode *assign = (AssignmentNode *)node;
        if (assign->variable)
        {
            add_variable(table, current_scope, assign->variable);
            VariableValue *value = create_variable_value_from_node(assign->value, table, current_scope);
            set_variable_value(table, current_scope, assign->variable, value);
        }
        set_var_table(assign->value, table, current_scope);
        break;
    }
    case NODE_EXPRESSION:
    {
        ExpressionNode *expr = (ExpressionNode *)node;
        set_var_table(expr->left, table, current_scope);
        set_var_table(expr->right, table, current_scope);
        break;
    }
    case NODE_CONDITION:
    {
        ConditionNode *cond = (ConditionNode *)node;
        set_var_table(cond->left, table, current_scope);
        set_var_table(cond->right, table, current_scope);
        break;
    }
    case NODE_IF_STATEMENT:
    {
        IfNode *if_node = (IfNode *)node;
        set_var_table(if_node->condition, table, current_scope);
        set_var_table(if_node->then_statement, table, current_scope);
        set_var_table(if_node->else_statement, table, current_scope);
        break;
    }
    case NODE_WHILE_STATEMENT:
    {
        WhileNode *while_node = (WhileNode *)node;
        set_var_table(while_node->condition, table, current_scope);
        set_var_table(while_node->body, table, current_scope);
        break;
    }
    case NODE_FOR_STATEMENT:
    {
        ForNode *for_node = (ForNode *)node;
        set_var_table(for_node->init, table, current_scope);
        set_var_table(for_node->condition, table, current_scope);
        set_var_table(for_node->increment, table, current_scope);
        set_var_table(for_node->body, table, current_scope);
        break;
    }
    case NODE_COMPOUND_STATEMENT:
    {
        CompoundNode *comp = (CompoundNode *)node;
        if (comp->statements)
        {
            set_var_table(comp->statements, table, current_scope);
        }
        break;
    }
    case NODE_RETURN:
    {
        ReturnNode *ret = (ReturnNode *)node;
        set_var_table(ret->value, table, current_scope);
        break;
    }
    case NODE_FUNCTION_DEF:
    {
        FunctionDefNode *func = (FunctionDefNode *)node;
        set_var_table(func->parameters, table, func->name);
        set_var_table(func->body, table, func->name);
        break;
    }
    case NODE_FUNCTION_CALL:
    {
        FunctionCallNode *call = (FunctionCallNode *)node;
        set_var_table(call->arguments, table, current_scope);
        break;
    }
    case NODE_BINARY_OP:
    {
        BinaryOpNode *bin = (BinaryOpNode *)node;
        set_var_table(bin->left, table, current_scope);
        set_var_table(bin->right, table, current_scope);
        break;
    }
    case NODE_UNARY_OP:
    {
        UnaryOpNode *unary = (UnaryOpNode *)node;
        set_var_table(unary->operand, table, current_scope);
        break;
    }
    default:
        break;
    }

    set_var_table(node->next, table, current_scope);
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

void constant_folding(ASTNode *node)
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
    case NODE_CONDITION:
    {
        ConditionNode *cn = (ConditionNode *)node;
        mark_subtree_dead(cn->left);
        mark_subtree_dead(cn->right);
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

    // mark_subtree_dead(node->next);
}

void reachability_analysis(ASTNode *node, VariableTable *table, char *current_scope)
{
    if (node == NULL)
        return;

    switch (node->type)
    {
    case NODE_PROGRAM:
    {
        ProgramNode *prog = (ProgramNode *)node;
        reachability_analysis(prog->statements, table, current_scope);
        break;
    }
    case NODE_IF_STATEMENT:
    {
        IfNode *if_node = (IfNode *)node;

        int *cond_true = is_condition_always_true(if_node->condition, table, current_scope);
        int *cond_false = NULL;

        if (cond_true && *cond_true)
        {
            if (if_node->else_statement)
            {
                mark_subtree_dead(if_node->else_statement);
            }
        }
        else
        {
            cond_false = is_condition_always_false(if_node->condition, table, current_scope);
            if (cond_false && *cond_false)
            {
                if (if_node->then_statement)
                {
                    mark_subtree_dead(if_node->then_statement);
                }
                if (!if_node->else_statement)
                {
                    mark_subtree_dead(node);
                }
            }
        }

        if (cond_true)
            free(cond_true);
        if (cond_false)
            free(cond_false);

        reachability_analysis(if_node->condition, table, current_scope);
        reachability_analysis(if_node->then_statement, table, current_scope);
        reachability_analysis(if_node->else_statement, table, current_scope);
        break;
    }
    case NODE_WHILE_STATEMENT:
    {
        WhileNode *while_node = (WhileNode *)node;

        int *cond_false = is_condition_always_false(while_node->condition, table, current_scope);

        if (cond_false && *cond_false)
        {
            if (while_node->body)
            {
                mark_subtree_dead(while_node->body);
            }
            node->is_dead_code = 1;
        }

        if (cond_false)
            free(cond_false);

        reachability_analysis(while_node->condition, table, current_scope);
        reachability_analysis(while_node->body, table, current_scope);
        break;
    }
    case NODE_FOR_STATEMENT:
    {
        ForNode *for_node = (ForNode *)node;

        if (for_node->condition)
        {
            int *cond_false = is_condition_always_false(for_node->condition, table, current_scope);

            if (cond_false && *cond_false)
            {
                if (for_node->body)
                {
                    mark_subtree_dead(for_node->body);
                }
                node->is_dead_code = 1;
            }

            if (cond_false)
                free(cond_false);
        }

        reachability_analysis(for_node->init, table, current_scope);
        reachability_analysis(for_node->condition, table, current_scope);
        reachability_analysis(for_node->increment, table, current_scope);
        reachability_analysis(for_node->body, table, current_scope);
        break;
    }
    case NODE_COMPOUND_STATEMENT:
    {
        CompoundNode *comp = (CompoundNode *)node;
        if (comp->statements)
        {
            reachability_analysis(comp->statements, table, current_scope);
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
        reachability_analysis(func->body, table, func->name);
        break;
    }
    case NODE_FUNCTION_CALL:
    {
        FunctionCallNode *call = (FunctionCallNode *)node;
        reachability_analysis(call->arguments, table, current_scope);
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

    reachability_analysis(node->next, table, current_scope);
}

void check_dse_table(DSETable *table)
{
    for (DSETable *aux = table->next; aux; aux = aux->next)
    {
        DSETable *entry = aux;

        if (entry->node->type == NODE_DECLARATION || entry->node->type == NODE_ASSIGNMENT)
        {
            DSETable *check = entry->next;
            if (entry->loop_hash)
            {
                DSETable *aux_check = table->next;
                while (aux_check)
                {
                    if (aux_check->loop_hash == entry->loop_hash)
                    {
                        break;
                    }
                    aux_check = aux_check->next;
                }
                check = aux_check;
            }

            while (check)
            {
                if (strcmp(check->name, entry->name) == 0 && check->node->type == NODE_IDENTIFIER)
                {
                    break;
                }
                check = check->next;
            }
            if (!check)
            {
                entry->node->is_dead_code = 1;
            }
        }

        if (entry->node->type == NODE_ASSIGNMENT)
        {
            DSETable *check = entry->next;
            while (check)
            {
                if (strcmp(check->name, entry->name) == 0)
                {
                    AssignmentNode *assign = (AssignmentNode *)check->node;
                    if (assign->op == OP_ASSIGN)
                    {
                        if (entry->loop_hash && !check->loop_hash)
                            break;
                        entry->node->is_dead_code = 1;
                    }
                    break;
                }
                check = check->next;
            }
        }
    }
}

void liveness_and_dead_store_elimination(ASTNode *node, DSETable **table, uint64_t loop_hash)
{
    if (node == NULL)
        return;

    switch (node->type)
    {
    case NODE_PROGRAM:
    {
        ProgramNode *prog = (ProgramNode *)node;
        DSETable *prog_table = malloc(sizeof(DSETable));
        prog_table->next = NULL;
        prog_table->node = NULL;

        DSETable *current = prog_table;

        liveness_and_dead_store_elimination(prog->statements, &current, loop_hash);

        check_dse_table(prog_table);

        break;
    }
    case NODE_DECLARATION:
    {
        DeclarationNode *decl = (DeclarationNode *)node;
        if (decl->name && !node->is_dead_code)
        {
            DSETable *new_entry = malloc(sizeof(DSETable));
            new_entry->name = decl->name;
            new_entry->loop_hash = loop_hash;
            new_entry->next = NULL;
            new_entry->node = node;
            (*table)->next = new_entry;
            *table = new_entry;
        }

        liveness_and_dead_store_elimination(decl->initial_value, table, loop_hash);
        break;
    }
    case NODE_ASSIGNMENT:
    {
        AssignmentNode *assign = (AssignmentNode *)node;
        if (assign->variable && !node->is_dead_code)
        {
            DSETable *new_entry = malloc(sizeof(DSETable));
            new_entry->name = assign->variable;
            new_entry->loop_hash = loop_hash;
            new_entry->next = NULL;
            new_entry->node = node;
            (*table)->next = new_entry;
            *table = new_entry;
        }
        liveness_and_dead_store_elimination(assign->value, table, loop_hash);
        break;
    }
    case NODE_IDENTIFIER:
    {
        IdentifierNode *id = (IdentifierNode *)node;
        if (id->name && !node->is_dead_code)
        {
            DSETable *new_entry = malloc(sizeof(DSETable));
            new_entry->loop_hash = loop_hash;
            new_entry->name = id->name;
            new_entry->next = NULL;
            new_entry->node = node;
            (*table)->next = new_entry;
            *table = new_entry;
        }
        break;
    }
    case NODE_EXPRESSION:
    {
        ExpressionNode *expr = (ExpressionNode *)node;
        liveness_and_dead_store_elimination(expr->left, table, loop_hash);
        liveness_and_dead_store_elimination(expr->right, table, loop_hash);
        break;
    }
    case NODE_CONDITION:
    {
        ConditionNode *cond = (ConditionNode *)node;
        liveness_and_dead_store_elimination(cond->left, table, loop_hash);
        liveness_and_dead_store_elimination(cond->right, table, loop_hash);
        break;
    }
    case NODE_IF_STATEMENT:
    {
        IfNode *if_node = (IfNode *)node;
        liveness_and_dead_store_elimination(if_node->condition, table, loop_hash);
        liveness_and_dead_store_elimination(if_node->then_statement, table, loop_hash);
        liveness_and_dead_store_elimination(if_node->else_statement, table, loop_hash);
        break;
    }
    case NODE_WHILE_STATEMENT:
    {
        WhileNode *while_node = (WhileNode *)node;
        liveness_and_dead_store_elimination(while_node->condition, table, while_node->loop_hash);
        liveness_and_dead_store_elimination(while_node->body, table, while_node->loop_hash);
        break;
    }
    case NODE_FOR_STATEMENT:
    {
        ForNode *for_node = (ForNode *)node;
        liveness_and_dead_store_elimination(for_node->init, table, for_node->loop_hash);
        liveness_and_dead_store_elimination(for_node->condition, table, for_node->loop_hash);
        liveness_and_dead_store_elimination(for_node->increment, table, for_node->loop_hash);
        liveness_and_dead_store_elimination(for_node->body, table, for_node->loop_hash);
        break;
    }
    case NODE_COMPOUND_STATEMENT:
    {
        CompoundNode *comp = (CompoundNode *)node;
        if (comp->statements)
        {
            liveness_and_dead_store_elimination(comp->statements, table, loop_hash);
        }
        break;
    }
    case NODE_RETURN:
    {
        ReturnNode *ret = (ReturnNode *)node;
        liveness_and_dead_store_elimination(ret->value, table, loop_hash);
        break;
    }
    case NODE_FUNCTION_DEF:
    {
        FunctionDefNode *func = (FunctionDefNode *)node;
        DSETable *func_table = malloc(sizeof(DSETable));
        func_table->next = NULL;
        func_table->node = NULL;

        DSETable *current = func_table;

        liveness_and_dead_store_elimination(func->parameters, &current, loop_hash);
        liveness_and_dead_store_elimination(func->body, &current, loop_hash);

        check_dse_table(func_table);
        break;
    }
    case NODE_FUNCTION_CALL:
    {
        FunctionCallNode *call = (FunctionCallNode *)node;
        liveness_and_dead_store_elimination(call->arguments, table, loop_hash);
        break;
    }
    case NODE_BINARY_OP:
    {
        BinaryOpNode *bin = (BinaryOpNode *)node;
        liveness_and_dead_store_elimination(bin->left, table, loop_hash);
        liveness_and_dead_store_elimination(bin->right, table, loop_hash);
        break;
    }
    case NODE_UNARY_OP:
    {
        UnaryOpNode *unary = (UnaryOpNode *)node;
        liveness_and_dead_store_elimination(unary->operand, table, loop_hash);
        break;
    }
    default:
        break;
    }

    liveness_and_dead_store_elimination(node->next, table, loop_hash);
}

void empty_blocks(ASTNode *node)
{
    if (node == NULL)
        return;

    switch (node->type)
    {
    case NODE_PROGRAM:
    {
        ProgramNode *prog = (ProgramNode *)node;
        empty_blocks(prog->statements);
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
            empty_blocks(comp->statements);
        }
        break;
    }
    case NODE_IF_STATEMENT:
    {
        IfNode *i = (IfNode *)node;
        empty_blocks(i->then_statement);
        empty_blocks(i->else_statement);
        break;
    }
    case NODE_WHILE_STATEMENT:
    {
        WhileNode *w = (WhileNode *)node;
        empty_blocks(w->body);
        break;
    }
    case NODE_FOR_STATEMENT:
    {
        ForNode *f = (ForNode *)node;
        empty_blocks(f->body);
        break;
    }
    case NODE_FUNCTION_DEF:
    {
        FunctionDefNode *func = (FunctionDefNode *)node;
        empty_blocks(func->body);
        break;
    }
    default:
        break;
    }

    empty_blocks(node->next);
}

int *is_condition_always_true(ASTNode *condition, VariableTable *table, char *current_scope)
{
    if (condition == NULL)
        return NULL;

    switch (condition->type)
    {
    case NODE_NUMBER:
    {
        NumberNode *num = (NumberNode *)condition;
        int *result = malloc(sizeof(int));
        if (!result)
            return NULL;
        *result = strcmp(num->value, "0") != 0;
        return result;
    }

    case NODE_CONDITION:
    {
        ConditionNode *c = (ConditionNode *)condition;
        int *a = NULL, *b = NULL;
        IdentifierNode *left_id = NULL, *right_id = NULL;
        if (c->left)
        {
            if (c->left->type == NODE_NUMBER)
            {
                int val = parse_number_value(((NumberNode *)c->left)->value);
                a = malloc(sizeof(int));
                if (a)
                    *a = val;
            }
            else if (c->left->type == NODE_IDENTIFIER)
            {
                left_id = (IdentifierNode *)c->left;
                VariableInfo *left_var = find_variable(table, current_scope, left_id->name);
                if (left_var && left_var->value && left_var->value->type == VALUE_TYPE_INT)
                {
                    a = malloc(sizeof(int));
                    if (a)
                        *a = left_var->value->number;
                }
            }
            else if (c->left->type == NODE_CONDITION)
            {
                a = is_condition_always_true(c->left, table, current_scope);
            }
        }

        if (c->right)
        {
            if (c->right->type == NODE_NUMBER)
            {
                int val = parse_number_value(((NumberNode *)c->right)->value);
                b = malloc(sizeof(int));
                if (b)
                    *b = val;
            }
            else if (c->right->type == NODE_IDENTIFIER)
            {
                right_id = (IdentifierNode *)c->right;
                VariableInfo *right_var = find_variable(table, current_scope, right_id->name);
                if (right_var && right_var->value && right_var->value->type == VALUE_TYPE_INT)
                {
                    b = malloc(sizeof(int));
                    if (b)
                        *b = right_var->value->number;
                }
            }
            else if (c->right->type == NODE_CONDITION)
            {
                b = is_condition_always_true(c->right, table, current_scope);
            }
        }

        if (left_id && right_id && (strcmp(left_id->name, right_id->name) == 0))
        {
            int *result = malloc(sizeof(int));
            if (!result)
                return NULL;
            *result = eval_relational(c->op, 1, 1) != 0;
            return result;
        }

        if (a == NULL || b == NULL)
        {
            return NULL;
        }

        int *result = malloc(sizeof(int));
        if (!result)
        {
            free(a);
            free(b);
            return NULL;
        }
        printf("a: %d | b: %d\n", *a, *b);
        *result = eval_relational(c->op, *a, *b) == 0;

        if (c->left && c->left->type == NODE_NUMBER)
            free(a);
        if (c->right && c->right->type == NODE_NUMBER)
            free(b);
        return result;
    }

    case NODE_UNARY_OP:
    {
        UnaryOpNode *u = (UnaryOpNode *)condition;
        if (u->op == OP_NOT && u->operand && u->operand->type == NODE_NUMBER)
        {
            long v = parse_number_value(((NumberNode *)u->operand)->value);
            int *result = malloc(sizeof(int));
            if (!result)
                return NULL;
            *result = (!v) != 0;
            return result;
        }
        return NULL;
    }

    default:
        return NULL;
    }
}

int *is_condition_always_false(ASTNode *condition, VariableTable *table, char *current_scope)
{
    if (condition == NULL)
        return NULL;

    switch (condition->type)
    {
    case NODE_NUMBER:
    {
        NumberNode *num = (NumberNode *)condition;
        int *result = malloc(sizeof(int));
        if (!result)
            return NULL;
        *result = strcmp(num->value, "0") == 0;
        return result;
    }

    case NODE_CONDITION:
    {
        ConditionNode *c = (ConditionNode *)condition;
        int *a = NULL, *b = NULL;
        IdentifierNode *left_id = NULL, *right_id = NULL;

        if (c->left)
        {
            if (c->left->type == NODE_NUMBER)
            {
                int val = parse_number_value(((NumberNode *)c->left)->value);
                a = malloc(sizeof(int));
                if (a)
                    *a = val;
            }
            else if (c->left->type == NODE_IDENTIFIER)
            {
                left_id = (IdentifierNode *)c->left;
                VariableInfo *left_var = find_variable(table, current_scope, left_id->name);
                if (left_var && left_var->value && left_var->value->type == VALUE_TYPE_INT)
                {
                    a = malloc(sizeof(int));
                    if (a)
                        *a = left_var->value->number;
                }
            }
            else if (c->left->type == NODE_CONDITION)
            {
                a = is_condition_always_false(c->left, table, current_scope);
            }
        }

        if (c->right)
        {
            if (c->right->type == NODE_NUMBER)
            {
                int val = parse_number_value(((NumberNode *)c->right)->value);
                b = malloc(sizeof(int));
                if (b)
                    *b = val;
            }
            else if (c->right->type == NODE_IDENTIFIER)
            {
                right_id = (IdentifierNode *)c->right;
                VariableInfo *right_var = find_variable(table, current_scope, right_id->name);
                if (right_var && right_var->value && right_var->value->type == VALUE_TYPE_INT)
                {
                    b = malloc(sizeof(int));
                    if (b)
                        *b = right_var->value->number;
                }
            }
            else if (c->right->type == NODE_CONDITION)
            {
                b = is_condition_always_false(c->right, table, current_scope);
            }
        }

        if (left_id && right_id && (strcmp(left_id->name, right_id->name) == 0))
        {
            int *result = malloc(sizeof(int));
            if (!result)
                return NULL;
            *result = eval_relational(c->op, 1, 1) != 0;
            return result;
        }

        if (a == NULL || b == NULL)
        {
            return NULL;
        }

        int *result = malloc(sizeof(int));
        if (!result)
        {
            free(a);
            free(b);
            return NULL;
        }

        *result = eval_relational(c->op, *a, *b) == 0;

        if (c->left && c->left->type == NODE_NUMBER)
            free(a);
        if (c->right && c->right->type == NODE_NUMBER)
            free(b);

        return result;
    }

    case NODE_UNARY_OP:
    {
        UnaryOpNode *u = (UnaryOpNode *)condition;
        if (u->op == OP_NOT && u->operand && u->operand->type == NODE_NUMBER)
        {
            long v = parse_number_value(((NumberNode *)u->operand)->value);
            int *result = malloc(sizeof(int));
            if (!result)
                return NULL;
            *result = (!v) == 0;
            return result;
        }
        return NULL;
    }

    default:
        return NULL;
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
    if (var)
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