#include "optimizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

static int is_number_node(ASTNode *n);
static long parse_number_value(const char *s);
static ASTNode *make_number(long value);
static ASTNode *fold_expression(ASTNode *node);
static ASTNode *fold_condition(ASTNode *node);
static void fold_in_tree(ASTNode *node);
static int is_pure_expression(ASTNode *expr);
static long eval_relational(Operator op, long a, long b);
static long eval_binary(Operator op, long a, long b);

void print_dse_table(DSETable *head);
VariableValue *evaluate_expression_value(ASTNode *expr, DSETable *table, char *scope);

void optimize_code(ASTNode *ast)
{
    DSETable *head = malloc(sizeof(DSETable));
    head->next = NULL;
    DSETable *tail = head;

    constant_folding(ast);

    set_var_table(ast, &head, &tail, 0, "global");

    reachability_analysis(ast, head, "global");
    liveness_and_dead_store_elimination(head);

    empty_blocks(ast);
    liveness_and_dead_store_elimination(head);

    remove_dead_code(ast);

    // print_dse_table(head);
    // print_optimization_report(ast);
    // free_variable_table(var_table);
}

const char *get_node_type_str(NodeType type)
{
    switch (type)
    {
    case NODE_PROGRAM:
        return "PROGRAM";
    case NODE_DECLARATION:
        return "DECLARATION";
    case NODE_ASSIGNMENT:
        return "ASSIGNMENT";
    case NODE_IDENTIFIER:
        return "IDENTIFIER";
    case NODE_NUMBER:
        return "NUMBER";
    case NODE_STRING_LITERAL:
        return "STRING";
    case NODE_EXPRESSION:
        return "EXPRESSION";
    case NODE_CONDITION:
        return "CONDITION";
    case NODE_IF_STATEMENT:
        return "IF";
    case NODE_WHILE_STATEMENT:
        return "WHILE";
    case NODE_FOR_STATEMENT:
        return "FOR";
    case NODE_COMPOUND_STATEMENT:
        return "COMPOUND";
    case NODE_RETURN:
        return "RETURN";
    case NODE_FUNCTION_DEF:
        return "FUNC_DEF";
    case NODE_FUNCTION_CALL:
        return "FUNC_CALL";
    case NODE_BINARY_OP:
        return "BINARY_OP";
    case NODE_UNARY_OP:
        return "UNARY_OP";
    default:
        return "UNKNOWN";
    }
}

void print_dse_table(DSETable *head)
{
    printf("\n=========================== DSE TABLE ===========================\n");
    printf("%-3s %-15s %-15s %-10s %-15s %-15s %-15s %-15s %-10s\n",
           "#", "Name", "Scope", "Type", "Value",
           "Loop Hash", "ID Hash", "Node Type", "Dead?");
    printf("------------------------------------------------------------------------------------------------------------------------------------\n");

    DSETable *current = head;
    int index = 1;

    while (current != NULL)
    {
        const char *type_str = "unknown";
        char value_str[128] = {0};

        if (current->value != NULL)
        {
            switch (current->value->type)
            {
            case VALUE_TYPE_INT:
                type_str = "int";
                snprintf(value_str, sizeof(value_str), "%d", current->value->number);
                break;

            case VALUE_TYPE_STRING:
                type_str = "string";
                snprintf(value_str, sizeof(value_str), "\"%s\"", current->value->str);
                break;

            case VALUE_TYPE_UNKNOWN:
            default:
                type_str = "unknown";
                snprintf(value_str, sizeof(value_str), "(unknown)");
                break;
            }
        }
        else
        {
            snprintf(value_str, sizeof(value_str), "(null)");
        }

        int dead_flag = 0;
        if (current->node)
            dead_flag = current->node->is_dead_code;

        printf("%-3d %-15s %-15s %-10s %-15s %-15" PRIu64 " %-15" PRIu64 " %-15s %-10s\n",
               index++,
               current->name ? current->name : "(null)",
               current->scope ? current->scope : "(null)",
               type_str,
               value_str,
               current->loop_hash,
               current->id_hash,
               current->node ? get_node_type_str(current->node->type) : "(null)",
               dead_flag ? "yes" : "no");

        current = current->next;
    }

    printf("====================================================================================================================================\n\n");
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

int eval_condition(ASTNode *condition, DSETable *head, char *scope)
{
    if (!condition)
        return -1;

    VariableValue *val = evaluate_expression_value(condition, head, scope);

    if (!val)
        return -1; // não foi possível resolver

    if (val->type == VALUE_TYPE_INT)
    {
        if (val->number != 0)
            return 1; // sempre verdadeiro
        else
            return 0; // sempre falso
    }

    return -1; // caso string
}

void reachability_analysis(ASTNode *node, DSETable *head, char *scope)
{
    if (node == NULL)
        return;

    switch (node->type)
    {
    case NODE_PROGRAM:
    {
        ProgramNode *prog = (ProgramNode *)node;
        reachability_analysis(prog->statements, head, scope);
        break;
    }
    case NODE_IF_STATEMENT:
    {
        IfNode *if_node = (IfNode *)node;
        int cond = eval_condition(if_node->condition, head, scope);
        if (cond == 1)
        {
            if (if_node->else_statement)
            {
                mark_subtree_dead(if_node->else_statement);
            }
        }
        else if (cond == 0)
        {
            if (if_node->condition) {
                mark_subtree_dead(if_node->condition);
            }
            if (if_node->then_statement)
            {
                mark_subtree_dead(if_node->then_statement);
            }
            if (!if_node->else_statement)
            {
                mark_subtree_dead(node);
            }
        }

        reachability_analysis(if_node->condition, head, scope);
        reachability_analysis(if_node->then_statement, head, scope);
        reachability_analysis(if_node->else_statement, head, scope);
        break;
    }
    case NODE_WHILE_STATEMENT:
    {
        WhileNode *while_node = (WhileNode *)node;
        int cond = eval_condition(while_node->condition, head, scope);

        if (cond == 0)
        {
            if (while_node->body)
            {
                mark_subtree_dead(while_node->body);
            }
            node->is_dead_code = 1;
        }

        reachability_analysis(while_node->condition, head, scope);
        reachability_analysis(while_node->body, head, scope);
        break;
    }
    case NODE_FOR_STATEMENT:
    {
        ForNode *for_node = (ForNode *)node;

        if (for_node->condition)
        {
            int cond = eval_condition(for_node->condition, head, scope);
            if (cond == 0)
            {
                if (for_node->body)
                {
                    mark_subtree_dead(for_node->body);
                }
                node->is_dead_code = 1;
            }
        }

        reachability_analysis(for_node->init, head, scope);
        reachability_analysis(for_node->condition, head, scope);
        reachability_analysis(for_node->increment, head, scope);
        reachability_analysis(for_node->body, head, scope);
        break;
    }
    case NODE_COMPOUND_STATEMENT:
    {
        CompoundNode *comp = (CompoundNode *)node;
        if (comp->statements)
        {
            reachability_analysis(comp->statements, head, scope);
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
        reachability_analysis(func->body, head, func->name);
        break;
    }
    case NODE_FUNCTION_CALL:
    {
        FunctionCallNode *call = (FunctionCallNode *)node;
        reachability_analysis(call->arguments, head, scope);
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

    reachability_analysis(node->next, head, scope);
}

void liveness_and_dead_store_elimination(DSETable *table)
{
    for (DSETable *aux = table->next; aux; aux = aux->next)
    {
        DSETable *entry = aux;

        // CASO: n = n;
        if (entry->node->type == NODE_ASSIGNMENT)
        {
            AssignmentNode *assig = (AssignmentNode *)entry->node;
            if (assig->value && assig->value->type == NODE_IDENTIFIER && assig->op == OP_ASSIGN)
            {
                IdentifierNode *id = (IdentifierNode *)assig->value;
                if (strcmp(id->name, assig->variable) == 0)
                {
                    entry->node->is_dead_code = 1;
                    continue;
                }
            }
        }

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
                if (strcmp(check->name, entry->name) == 0 && strcmp(check->scope, entry->scope) == 0 && check->node->type == NODE_IDENTIFIER && !check->node->is_dead_code)
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
                if (strcmp(check->name, entry->name) == 0 && strcmp(check->scope, entry->scope) == 0 && !check->node->is_dead_code)
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

VariableValue *resolve_final_value(DSETable *head, const char *name, const char *scope)
{
    DSETable *current = head;
    DSETable *last_match = NULL;

    // Encontra a última atribuição para o nome dentro do mesmo escopo
    while (current != NULL)
    {
        if (current->scope && scope && strcmp(current->scope, scope) == 0 &&
            strcmp(current->name, name) == 0 && !current->node->is_dead_code)
        {
            last_match = current;
        }
        current = current->next;
    }

    if (last_match == NULL || last_match->value == NULL)
        return NULL;

    // printf("last math %s val t %d\n", name, last_match->value->number);
    VariableValue *val = last_match->value;

    // Se o valor for literal (int ou string), retorna
    if (val->type == VALUE_TYPE_INT || val->type == VALUE_TYPE_STRING)
        return val;

    // Se o valor for NODE_IDENTIFIER (encadeamento), resolve recursivamente
    if (last_match->node && last_match->node->type == NODE_DECLARATION)
    {
        DeclarationNode *decl = (DeclarationNode *)last_match->node;
        if (decl->initial_value && decl->initial_value->type == NODE_IDENTIFIER)
        {
            IdentifierNode *id = (IdentifierNode *)decl->initial_value;
            return resolve_final_value(head, id->name, scope);
        }
    }

    // Se não for número, string nem ID, retorna NULL
    return NULL;
}

VariableValue *evaluate_expression_value(ASTNode *expr, DSETable *table, char *scope)
{
    if (!expr)
        return NULL;

    switch (expr->type)
    {
    case NODE_NUMBER:
    {
        NumberNode *num = (NumberNode *)expr;
        VariableValue *val = malloc(sizeof(VariableValue));
        val->type = VALUE_TYPE_INT;
        val->number = atoi(num->value);
        return val;
    }

    case NODE_STRING_LITERAL:
    {
        StringLiteralNode *str = (StringLiteralNode *)expr;
        VariableValue *val = malloc(sizeof(VariableValue));
        val->type = VALUE_TYPE_STRING;
        val->str = str->value;
        return val;
    }

    case NODE_IDENTIFIER:
    {
        IdentifierNode *id = (IdentifierNode *)expr;

        VariableValue *r = malloc(sizeof(VariableValue));
        return resolve_final_value(table, id->name, scope);
    }

    case NODE_CONDITION:
    case NODE_BINARY_OP:
    {
        BinaryOpNode *bin = (BinaryOpNode *)expr;
        VariableValue *left_val = evaluate_expression_value(bin->left, table, scope);
        VariableValue *right_val = evaluate_expression_value(bin->right, table, scope);

        // CASO: ... || 1
        if (right_val && right_val->type == VALUE_TYPE_INT && bin->op == OP_OR && right_val->number > 0)
        {
            return right_val;
        }
        // CASO: 1 || ...
        if (left_val && left_val->type == VALUE_TYPE_INT && bin->op == OP_OR && left_val->number > 0)
        {
            return left_val;
        }

        // CASO: ... && 0
        if (right_val && right_val->type == VALUE_TYPE_INT && bin->op == OP_AND && right_val->number == 0)
        {
            return right_val;
        }
        // CASO: 0 && ...
        if (left_val && left_val->type == VALUE_TYPE_INT && bin->op == OP_AND && left_val->number == 0)
        {
            return left_val;
        }

        if (!left_val || !right_val)
            return NULL;

        // Só tratamos inteiros aqui — strings você pode definir outra semântica depois
        if (left_val->type != VALUE_TYPE_INT || right_val->type != VALUE_TYPE_INT)
            return NULL;

        VariableValue *result = malloc(sizeof(VariableValue));
        result->type = VALUE_TYPE_INT;

        switch (bin->op)
        {
        case OP_ADD:
            result->number = left_val->number + right_val->number;
            break;
        case OP_SUB:
            result->number = left_val->number - right_val->number;
            break;
        case OP_MUL:
            result->number = left_val->number * right_val->number;
            break;
        case OP_DIV:
            result->number = (right_val->number != 0) ? left_val->number / right_val->number : 0;
            break;
        // case '%':
        //     result->number = (right_val->number != 0) ? left_val->number % right_val->number : 0;
        //     break;
        case OP_GT:
            result->number = left_val->number > right_val->number;
            break;
        case OP_LT:
            result->number = left_val->number < right_val->number;
            break;
        case OP_EQ:
            result->number = (left_val->number == right_val->number);
            break;
        case OP_AND:
            result->number = (left_val->number && right_val->number);
            break;
        case OP_OR:
            result->number = (left_val->number || right_val->number);
            break;
        default:
            free(result);
            result = NULL;
            break;
        }

        return result;
    }

    default:
        return NULL;
    }
}

void set_var_table(ASTNode *node, DSETable **head, DSETable **tail, uint64_t loop_hash, char *scope)
{
    if (node == NULL)
        return;

    switch (node->type)
    {
    case NODE_PROGRAM:
    {
        ProgramNode *prog = (ProgramNode *)node;
        set_var_table(prog->statements, head, tail, loop_hash, scope);
        break;
    }
    case NODE_DECLARATION:
    {
        DeclarationNode *decl = (DeclarationNode *)node;
        if (decl->name)
        {
            DSETable *new_entry = malloc(sizeof(DSETable));
            new_entry->name = decl->name;
            new_entry->id_hash = decl->hash;
            new_entry->loop_hash = loop_hash;
            new_entry->scope = scope;
            new_entry->next = NULL;
            new_entry->value = NULL;
            new_entry->node = node;
            (*tail)->next = new_entry;
            *tail = new_entry;

            if (decl->initial_value && loop_hash == 0) // TODO: incluir optimizacoes dentro de loops
            {
                VariableValue *val = evaluate_expression_value(decl->initial_value, *head, scope);
                if (val)
                {
                    new_entry->value = malloc(sizeof(VariableValue));
                    memcpy(new_entry->value, val, sizeof(VariableValue));
                }
            }
        }

        set_var_table(decl->initial_value, head, tail, loop_hash, scope);
        break;
    }
    case NODE_ASSIGNMENT:
    {
        AssignmentNode *assign = (AssignmentNode *)node;
        if (assign->variable)
        {
            DSETable *new_entry = malloc(sizeof(DSETable));
            new_entry->name = assign->variable;
            new_entry->id_hash = assign->hash;
            new_entry->loop_hash = loop_hash;
            new_entry->scope = scope;
            new_entry->next = NULL;
            new_entry->value = NULL;
            new_entry->node = node;
            (*tail)->next = new_entry;
            *tail = new_entry;

            if (assign->value && loop_hash == 0) // TODO: incluir optimizacoes dentro de loops
            {
                VariableValue *val = evaluate_expression_value(assign->value, *head, scope);
                if (val)
                {
                    new_entry->value = malloc(sizeof(VariableValue));
                    memcpy(new_entry->value, val, sizeof(VariableValue));
                }
            }
        }
        set_var_table(assign->value, head, tail, loop_hash, scope);
        break;
    }
    case NODE_IDENTIFIER:
    {
        IdentifierNode *id = (IdentifierNode *)node;
        if (id->name)
        {
            VariableValue *val = resolve_final_value(*head, id->name, scope);

            DSETable *new_entry = malloc(sizeof(DSETable));
            new_entry->loop_hash = loop_hash;
            new_entry->name = id->name;
            new_entry->id_hash = id->hash;
            new_entry->scope = scope;
            new_entry->next = NULL;
            new_entry->value = NULL;
            new_entry->node = node;
            (*tail)->next = new_entry;
            *tail = new_entry;

            if (val)
            {
                new_entry->value = malloc(sizeof(VariableValue));
                memcpy(new_entry->value, val, sizeof(VariableValue));
            }
        }
        break;
    }
    case NODE_EXPRESSION:
    {
        ExpressionNode *expr = (ExpressionNode *)node;
        set_var_table(expr->left, head, tail, loop_hash, scope);
        set_var_table(expr->right, head, tail, loop_hash, scope);
        break;
    }
    case NODE_CONDITION:
    {
        ConditionNode *cond = (ConditionNode *)node;
        set_var_table(cond->left, head, tail, loop_hash, scope);
        set_var_table(cond->right, head, tail, loop_hash, scope);
        break;
    }
    case NODE_IF_STATEMENT:
    {
        IfNode *if_node = (IfNode *)node;
        set_var_table(if_node->condition, head, tail, loop_hash, scope);
        set_var_table(if_node->then_statement, head, tail, loop_hash, scope);
        set_var_table(if_node->else_statement, head, tail, loop_hash, scope);
        break;
    }
    case NODE_WHILE_STATEMENT:
    {
        WhileNode *while_node = (WhileNode *)node;
        set_var_table(while_node->condition, head, tail, while_node->loop_hash, scope);
        set_var_table(while_node->body, head, tail, while_node->loop_hash, scope);
        break;
    }
    case NODE_FOR_STATEMENT:
    {
        ForNode *for_node = (ForNode *)node;
        set_var_table(for_node->init, head, tail, for_node->loop_hash, scope);
        set_var_table(for_node->condition, head, tail, for_node->loop_hash, scope);
        set_var_table(for_node->increment, head, tail, for_node->loop_hash, scope);
        set_var_table(for_node->body, head, tail, for_node->loop_hash, scope);
        break;
    }
    case NODE_COMPOUND_STATEMENT:
    {
        CompoundNode *comp = (CompoundNode *)node;
        if (comp->statements)
        {
            set_var_table(comp->statements, head, tail, loop_hash, scope);
        }
        break;
    }
    case NODE_RETURN:
    {
        ReturnNode *ret = (ReturnNode *)node;
        set_var_table(ret->value, head, tail, loop_hash, scope);
        break;
    }
    case NODE_FUNCTION_DEF:
    {
        FunctionDefNode *func = (FunctionDefNode *)node;
        set_var_table(func->parameters, head, tail, loop_hash, func->name);
        set_var_table(func->body, head, tail, loop_hash, func->name);
        break;
    }
    case NODE_FUNCTION_CALL:
    {
        FunctionCallNode *call = (FunctionCallNode *)node;
        set_var_table(call->arguments, head, tail, loop_hash, scope);
        break;
    }
    case NODE_BINARY_OP:
    {
        BinaryOpNode *bin = (BinaryOpNode *)node;
        set_var_table(bin->left, head, tail, loop_hash, scope);
        set_var_table(bin->right, head, tail, loop_hash, scope);
        break;
    }
    case NODE_UNARY_OP:
    {
        UnaryOpNode *unary = (UnaryOpNode *)node;
        set_var_table(unary->operand, head, tail, loop_hash, scope);
        break;
    }
    default:
        break;
    }

    set_var_table(node->next, head, tail, loop_hash, scope);
}

int all_statements_dead(ASTNode *stmt_list)
{
    ASTNode *current = stmt_list;

    while (current)
    {
        if (!current->is_dead_code)
            return 0; // encontrou um statement vivo
        current = current->next;
    }

    return 1; // se não encontrou nenhum, considera vazio (morto também)
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
            if (all_statements_dead(comp->statements))
                node->is_dead_code = 1;
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

// void free_variable_table(VariableTable *table)
// {
//     for (int i = 0; i < table->count; i++)
//     {
//         free(table->variables[i].name);
//     }
//     free(table->variables);
//     free(table);
// }

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