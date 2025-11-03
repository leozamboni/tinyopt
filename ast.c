#include "ast.h"

// Funções para criar nós
ASTNode* create_program_node() {
    ProgramNode *node = malloc(sizeof(ProgramNode));
    node->base.type = NODE_PROGRAM;
    node->base.next = NULL;
    node->base.parent = NULL;
    node->base.line = 0;
    node->base.is_dead_code = 0;
    node->statements = NULL;
    return (ASTNode*)node;
}

ASTNode* create_declaration_node(DataType type, char *name, ASTNode *init, int array_size) {
    DeclarationNode *node = malloc(sizeof(DeclarationNode));
    node->base.type = NODE_DECLARATION;
    node->base.next = NULL;
    node->base.parent = NULL;
    node->base.line = 0;
    node->base.is_dead_code = 0;
    node->data_type = type;
    node->name = strdup(name);
    node->initial_value = init;
    node->array_size = array_size;
    return (ASTNode*)node;
}

ASTNode* create_assignment_node(char *var, Operator op, ASTNode *value) {
    AssignmentNode *node = malloc(sizeof(AssignmentNode));
    node->base.type = NODE_ASSIGNMENT;
    node->base.next = NULL;
    node->base.parent = NULL;
    node->base.line = 0;
    node->base.is_dead_code = 0;
    node->variable = strdup(var);
    node->op = op;
    node->value = value;
    return (ASTNode*)node;
}

ASTNode* create_expression_node(Operator op, ASTNode *left, ASTNode *right, char *value) {
    ExpressionNode *node = malloc(sizeof(ExpressionNode));
    node->base.type = NODE_EXPRESSION;
    node->base.next = NULL;
    node->base.parent = NULL;
    node->base.line = 0;
    node->base.is_dead_code = 0;
    node->op = op;
    node->left = left;
    node->right = right;
    node->value = value ? strdup(value) : NULL;
    return (ASTNode*)node;
}

ASTNode* create_condition_node(Operator op, ASTNode *left, ASTNode *right) {
    ConditionNode *node = malloc(sizeof(ConditionNode));
    node->base.type = NODE_CONDITION;
    node->base.next = NULL;
    node->base.parent = NULL;
    node->base.line = 0;
    node->base.is_dead_code = 0;
    node->op = op;
    node->left = left;
    node->right = right;
    return (ASTNode*)node;
}

ASTNode* create_if_node(ASTNode *condition, ASTNode *then_stmt, ASTNode *else_stmt) {
    IfNode *node = malloc(sizeof(IfNode));
    node->base.type = NODE_IF_STATEMENT;
    node->base.next = NULL;
    node->base.parent = NULL;
    node->base.line = 0;
    node->base.is_dead_code = 0;
    node->condition = condition;
    node->then_statement = then_stmt;
    node->else_statement = else_stmt;
    return (ASTNode*)node;
}

ASTNode* create_while_node(ASTNode *condition, ASTNode *body, uint64_t loop_hash) {
    WhileNode *node = malloc(sizeof(WhileNode));
    node->base.type = NODE_WHILE_STATEMENT;
    node->base.next = NULL;
    node->base.parent = NULL;
    node->base.line = 0;
    node->base.is_dead_code = 0;
    node->loop_hash = loop_hash;
    node->condition = condition;
    node->body = body;
    return (ASTNode*)node;
}

ASTNode* create_for_node(ASTNode *init, ASTNode *condition, ASTNode *increment, ASTNode *body, uint64_t loop_hash) {
    ForNode *node = malloc(sizeof(ForNode));
    node->base.type = NODE_FOR_STATEMENT;
    node->base.next = NULL;
    node->base.parent = NULL;
    node->base.line = 0;
    node->base.is_dead_code = 0;
    node->init = init;
    node->condition = condition;
    node->increment = increment;
    node->body = body;
    node->loop_hash = loop_hash;
    return (ASTNode*)node;
}

ASTNode* create_compound_node(ASTNode *statements) {
    CompoundNode *node = malloc(sizeof(CompoundNode));
    if (!node) {
        return NULL;
    }
    node->base.type = NODE_COMPOUND_STATEMENT;
    node->base.next = NULL;
    node->base.parent = NULL;
    node->base.line = 0;
    node->base.is_dead_code = 0;
    node->statements = statements;
    return (ASTNode*)node;
}

ASTNode* create_return_node(ASTNode *value) {
    ReturnNode *node = malloc(sizeof(ReturnNode));
    node->base.type = NODE_RETURN;
    node->base.next = NULL;
    node->base.parent = NULL;
    node->base.line = 0;
    node->base.is_dead_code = 0;
    node->value = value;
    return (ASTNode*)node;
}

ASTNode* create_control_node(int is_break) {
    ControlNode *node = malloc(sizeof(ControlNode));
    node->base.type = is_break ? NODE_BREAK : NODE_CONTINUE;
    node->base.next = NULL;
    node->base.parent = NULL;
    node->base.line = 0;
    node->base.is_dead_code = 0;
    node->is_break = is_break;
    return (ASTNode*)node;
}

ASTNode* create_identifier_node(char *name) {
    IdentifierNode *node = malloc(sizeof(IdentifierNode));
    node->base.type = NODE_IDENTIFIER;
    node->base.next = NULL;
    node->base.parent = NULL;
    node->base.line = 0;
    node->base.is_dead_code = 0;
    node->name = strdup(name);
    return (ASTNode*)node;
}

ASTNode* create_number_node(char *value, DataType type) {
    NumberNode *node = malloc(sizeof(NumberNode));
    node->base.type = NODE_NUMBER;
    node->base.next = NULL;
    node->base.parent = NULL;
    node->base.line = 0;
    node->base.is_dead_code = 0;
    node->value = strdup(value);
    node->data_type = type;
    return (ASTNode*)node;
}

ASTNode* create_char_literal_node(char *value) {
    CharLiteralNode *node = malloc(sizeof(CharLiteralNode));
    node->base.type = NODE_CHAR_LITERAL;
    node->base.next = NULL;
    node->base.parent = NULL;
    node->base.line = 0;
    node->base.is_dead_code = 0;
    node->value = strdup(value);
    return (ASTNode*)node;
}

ASTNode* create_string_literal_node(char *value) {
    StringLiteralNode *node = malloc(sizeof(StringLiteralNode));
    node->base.type = NODE_STRING_LITERAL;
    node->base.next = NULL;
    node->base.parent = NULL;
    node->base.line = 0;
    node->base.is_dead_code = 0;
    node->value = strdup(value);
    return (ASTNode*)node;
}

ASTNode* create_binary_op_node(Operator op, ASTNode *left, ASTNode *right) {
    BinaryOpNode *node = malloc(sizeof(BinaryOpNode));
    node->base.type = NODE_BINARY_OP;
    node->base.next = NULL;
    node->base.parent = NULL;
    node->base.line = 0;
    node->base.is_dead_code = 0;
    node->op = op;
    node->left = left;
    node->right = right;
    return (ASTNode*)node;
}

ASTNode* create_unary_op_node(Operator op, ASTNode *operand) {
    UnaryOpNode *node = malloc(sizeof(UnaryOpNode));
    node->base.type = NODE_UNARY_OP;
    node->base.next = NULL;
    node->base.parent = NULL;
    node->base.line = 0;
    node->base.is_dead_code = 0;
    node->op = op;
    node->operand = operand;
    return (ASTNode*)node;
}

ASTNode* create_function_def_node(DataType return_type, char *name, ASTNode *parameters, ASTNode *body) {
    FunctionDefNode *node = malloc(sizeof(FunctionDefNode));
    node->base.type = NODE_FUNCTION_DEF;
    node->base.next = NULL;
    node->base.parent = NULL;
    node->base.line = 0;
    node->base.is_dead_code = 0;
    node->return_type = return_type;
    node->name = strdup(name);
    node->parameters = parameters;
    node->body = body;
    return (ASTNode*)node;
}

ASTNode *create_function_call_node(char *name, ASTNode *arguments) {
    FunctionCallNode *node = malloc(sizeof(FunctionCallNode));
    node->base.type = NODE_FUNCTION_CALL;
    node->name = strdup(name);
    node->arguments = arguments;
    return (ASTNode *)node;
}


ASTNode* create_parameter_list_node(ASTNode *parameters) {
    ParameterListNode *node = malloc(sizeof(ParameterListNode));
    node->base.type = NODE_PARAMETER_LIST;
    node->base.next = NULL;
    node->base.parent = NULL;
    node->base.line = 0;
    node->base.is_dead_code = 0;
    node->parameters = parameters;
    return (ASTNode*)node;
}

// Funções para manipular a AST
void add_statement(ASTNode *program, ASTNode *statement) {
    if (program->type != NODE_PROGRAM) {
        return;
    }
    
    ProgramNode *prog = (ProgramNode*)program;
    if (prog->statements == NULL) {
        prog->statements = statement;
    } else {
        ASTNode *current = prog->statements;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = statement;
    }
}

ASTNode *add_args(ASTNode *list, ASTNode *expr) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node = expr = expr;
    node->next = NULL;

    if (list == NULL) {
        return node;
    }
    
    ASTNode *aux = list;
    while (aux->next != NULL)
        aux = aux->next;
    aux->next = node;

    return list;
}

void free_ast(ASTNode *node) {
    if (node == NULL) return;
    
    // Liberar nós filhos primeiro
    switch (node->type) {
        case NODE_PROGRAM: {
            ProgramNode *prog = (ProgramNode*)node;
            free_ast(prog->statements);
            break;
        }
        case NODE_DECLARATION: {
            DeclarationNode *decl = (DeclarationNode*)node;
            free(decl->name);
            free_ast(decl->initial_value);
            break;
        }
        case NODE_ASSIGNMENT: {
            AssignmentNode *assign = (AssignmentNode*)node;
            free(assign->variable);
            free_ast(assign->value);
            break;
        }
        case NODE_EXPRESSION: {
            ExpressionNode *expr = (ExpressionNode*)node;
            free_ast(expr->left);
            free_ast(expr->right);
            if (expr->value) free(expr->value);
            break;
        }
        case NODE_CONDITION: {
            ConditionNode *cond = (ConditionNode*)node;
            free_ast(cond->left);
            free_ast(cond->right);
            break;
        }
        case NODE_IF_STATEMENT: {
            IfNode *if_node = (IfNode*)node;
            free_ast(if_node->condition);
            free_ast(if_node->then_statement);
            free_ast(if_node->else_statement);
            break;
        }
        case NODE_WHILE_STATEMENT: {
            WhileNode *while_node = (WhileNode*)node;
            free_ast(while_node->condition);
            free_ast(while_node->body);
            break;
        }
        case NODE_FOR_STATEMENT: {
            ForNode *for_node = (ForNode*)node;
            free_ast(for_node->init);
            free_ast(for_node->condition);
            free_ast(for_node->increment);
            free_ast(for_node->body);
            break;
        }
        case NODE_COMPOUND_STATEMENT: {
            CompoundNode *comp = (CompoundNode*)node;
            if (comp->statements) {
                free_ast(comp->statements);
            }
            break;
        }
        case NODE_RETURN: {
            ReturnNode *ret = (ReturnNode*)node;
            free_ast(ret->value);
            break;
        }
        case NODE_IDENTIFIER: {
            IdentifierNode *id = (IdentifierNode*)node;
            free(id->name);
            break;
        }
        case NODE_NUMBER: {
            NumberNode *num = (NumberNode*)node;
            free(num->value);
            break;
        }
        case NODE_CHAR_LITERAL: {
            CharLiteralNode *chr = (CharLiteralNode*)node;
            free(chr->value);
            break;
        }
        case NODE_STRING_LITERAL: {
            StringLiteralNode *str = (StringLiteralNode*)node;
            free(str->value);
            break;
        }
        case NODE_BINARY_OP: {
            BinaryOpNode *bin = (BinaryOpNode*)node;
            free_ast(bin->left);
            free_ast(bin->right);
            break;
        }
        case NODE_UNARY_OP: {
            UnaryOpNode *unary = (UnaryOpNode*)node;
            free_ast(unary->operand);
            break;
        }
        case NODE_FUNCTION_DEF: {
            FunctionDefNode *func = (FunctionDefNode*)node;
            free(func->name);
            free_ast(func->parameters);
            free_ast(func->body);
            break;
        }
        case NODE_FUNCTION_CALL: {
            FunctionCallNode *call = (FunctionCallNode*)node;
            free(call->name);
            free_ast(call->arguments);
            break;
        }
        case NODE_PARAMETER_LIST: {
            ParameterListNode *params = (ParameterListNode*)node;
            free_ast(params->parameters);
            break;
        }
        default:
            break;
    }
    
    // Liberar próximo nó na lista
    free_ast(node->next);
    
    // Liberar o nó atual
    free(node);
}

void print_ast(ASTNode *node, int depth) {
    if (node == NULL) return;
    
    // Indentação
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
    
    // Marcar código morto
    if (node->is_dead_code) {
        printf("[DEAD] ");
    }
    
    switch (node->type) {
        case NODE_PROGRAM:
            printf("Program\n");
            print_ast(((ProgramNode*)node)->statements, depth + 1);
            break;
        case NODE_DECLARATION: {
            DeclarationNode *decl = (DeclarationNode*)node;
            printf("Declaration: %s\n", decl->name);
            if (decl->initial_value) {
                print_ast(decl->initial_value, depth + 1);
            }
            break;
        }
        case NODE_ASSIGNMENT: {
            AssignmentNode *assign = (AssignmentNode*)node;
            printf("Assignment: %s\n", assign->variable);
            print_ast(assign->value, depth + 1);
            break;
        }
        case NODE_EXPRESSION: {
            ExpressionNode *expr = (ExpressionNode*)node;
            if (expr->value) {
                printf("Expression: %s\n", expr->value);
            } else {
                printf("Expression: Binary Op\n");
                print_ast(expr->left, depth + 1);
                print_ast(expr->right, depth + 1);
            }
            break;
        }
        case NODE_CONDITION: {
            ConditionNode *cond = (ConditionNode*)node;
            printf("Condition\n");
            print_ast(cond->left, depth + 1);
            print_ast(cond->right, depth + 1);
            break;
        }
        case NODE_IF_STATEMENT: {
            IfNode *if_node = (IfNode*)node;
            printf("If Statement\n");
            print_ast(if_node->condition, depth + 1);
            print_ast(if_node->then_statement, depth + 1);
            if (if_node->else_statement) {
                print_ast(if_node->else_statement, depth + 1);
            }
            break;
        }
        case NODE_WHILE_STATEMENT: {
            WhileNode *while_node = (WhileNode*)node;
            printf("While Statement\n");
            print_ast(while_node->condition, depth + 1);
            print_ast(while_node->body, depth + 1);
            break;
        }
        case NODE_FOR_STATEMENT: {
            ForNode *for_node = (ForNode*)node;
            printf("For Statement\n");
            print_ast(for_node->init, depth + 1);
            print_ast(for_node->condition, depth + 1);
            print_ast(for_node->increment, depth + 1);
            print_ast(for_node->body, depth + 1);
            break;
        }
        case NODE_COMPOUND_STATEMENT: {
            CompoundNode *comp = (CompoundNode*)node;
            printf("Compound Statement\n");
            if (comp->statements) {
                print_ast(comp->statements, depth + 1);
            }
            break;
        }
        case NODE_RETURN:
            printf("Return\n");
            print_ast(((ReturnNode*)node)->value, depth + 1);
            break;
        case NODE_BREAK:
            printf("Break\n");
            break;
        case NODE_CONTINUE:
            printf("Continue\n");
            break;
        case NODE_IDENTIFIER:
            printf("Identifier: %s\n", ((IdentifierNode*)node)->name);
            break;
        case NODE_NUMBER:
            printf("Number: %s\n", ((NumberNode*)node)->value);
            break;
        case NODE_CHAR_LITERAL:
            printf("Char: %s\n", ((CharLiteralNode*)node)->value);
            break;
        case NODE_STRING_LITERAL:
            printf("String: %s\n", ((StringLiteralNode*)node)->value);
            break;
        case NODE_BINARY_OP: {
            BinaryOpNode *bin = (BinaryOpNode*)node;
            printf("Binary Op\n");
            print_ast(bin->left, depth + 1);
            print_ast(bin->right, depth + 1);
            break;
        }
        case NODE_UNARY_OP: {
            UnaryOpNode *unary = (UnaryOpNode*)node;
            printf("Unary Op\n");
            print_ast(unary->operand, depth + 1);
            break;
        }
        case NODE_FUNCTION_DEF: {
            FunctionDefNode *func = (FunctionDefNode*)node;
            const char *ret_type = (func->return_type == TYPE_INT) ? "int" :
                                   (func->return_type == TYPE_FLOAT) ? "float" :
                                   (func->return_type == TYPE_CHAR) ? "char" : "void";
            printf("Function Definition: %s %s\n", ret_type, func->name);
            if (func->parameters) {
                print_ast(func->parameters, depth + 1);
            }
            if (func->body) {
                print_ast(func->body, depth + 1);
            }
            break;
        }
        case NODE_FUNCTION_CALL: {
            FunctionCallNode *call = (FunctionCallNode*)node;
            printf("Function Call: %s\n", call->name);
            if (call->arguments) {
                print_ast(call->arguments, depth + 1);
            }
            break;
        }
        case NODE_PARAMETER_LIST: {
            ParameterListNode *params = (ParameterListNode*)node;
            printf("Parameter List\n");
            if (params->parameters) {
                print_ast(params->parameters, depth + 1);
            }
            break;
        }
        default:
            printf("Unknown Node Type\n");
            break;
    }
    
    // Processar próximo nó na lista
    print_ast(node->next, depth);
} 