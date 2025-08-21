#ifndef AST_H
#define AST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Tipos de nós da AST
typedef enum {
    NODE_PROGRAM,
    NODE_DECLARATION,
    NODE_ASSIGNMENT,
    NODE_EXPRESSION,
    NODE_CONDITION,
    NODE_IF_STATEMENT,
    NODE_WHILE_STATEMENT,
    NODE_FOR_STATEMENT,
    NODE_COMPOUND_STATEMENT,
    NODE_RETURN,
    NODE_BREAK,
    NODE_CONTINUE,
    NODE_IDENTIFIER,
    NODE_NUMBER,
    NODE_CHAR_LITERAL,
    NODE_STRING_LITERAL,
    NODE_BINARY_OP,
    NODE_UNARY_OP
} NodeType;

// Tipos de dados
typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_CHAR,
    TYPE_VOID,
    TYPE_ARRAY
} DataType;

// Operadores
typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_EQ,
    OP_NE,
    OP_LT,
    OP_LE,
    OP_GT,
    OP_GE,
    OP_AND,
    OP_OR,
    OP_NOT,
    OP_ASSIGN,
    OP_ADD_ASSIGN,
    OP_SUB_ASSIGN,
    OP_MUL_ASSIGN,
    OP_DIV_ASSIGN,
    OP_MOD_ASSIGN,
    OP_INC,
    OP_DEC
} Operator;

// Estrutura base do nó
typedef struct ASTNode {
    NodeType type;
    struct ASTNode *next;  // Para lista de declarações/instruções
    struct ASTNode *parent;
    int line;              // Linha no código fonte
    int is_dead_code;      // Flag para código morto
} ASTNode;

// Nó de programa
typedef struct {
    ASTNode base;
    struct ASTNode *statements;
} ProgramNode;

// Nó de declaração
typedef struct {
    ASTNode base;
    DataType data_type;
    char *name;
    struct ASTNode *initial_value;
    int array_size;
} DeclarationNode;

// Nó de atribuição
typedef struct {
    ASTNode base;
    char *variable;
    Operator op;
    struct ASTNode *value;
} AssignmentNode;

// Nó de expressão
typedef struct {
    ASTNode base;
    Operator op;
    struct ASTNode *left;
    struct ASTNode *right;
    char *value;  // Para literais
} ExpressionNode;

// Nó de condição
typedef struct {
    ASTNode base;
    Operator op;
    struct ASTNode *left;
    struct ASTNode *right;
} ConditionNode;

// Nó de if
typedef struct {
    ASTNode base;
    struct ASTNode *condition;
    struct ASTNode *then_statement;
    struct ASTNode *else_statement;
} IfNode;

// Nó de while
typedef struct {
    ASTNode base;
    struct ASTNode *condition;
    struct ASTNode *body;
} WhileNode;

// Nó de for
typedef struct {
    ASTNode base;
    struct ASTNode *init;
    struct ASTNode *condition;
    struct ASTNode *increment;
    struct ASTNode *body;
} ForNode;

// Nó de bloco composto
typedef struct {
    ASTNode base;
    struct ASTNode *statements;
} CompoundNode;

// Nó de return
typedef struct {
    ASTNode base;
    struct ASTNode *value;
} ReturnNode;

// Nó de break/continue
typedef struct {
    ASTNode base;
    int is_break;  // 1 para break, 0 para continue
} ControlNode;

// Nó de identificador
typedef struct {
    ASTNode base;
    char *name;
} IdentifierNode;

// Nó de número
typedef struct {
    ASTNode base;
    char *value;
    DataType data_type;
} NumberNode;

// Nó de literal de caractere
typedef struct {
    ASTNode base;
    char *value;
} CharLiteralNode;

// Nó de literal de string
typedef struct {
    ASTNode base;
    char *value;
} StringLiteralNode;

// Nó de operação binária
typedef struct {
    ASTNode base;
    Operator op;
    struct ASTNode *left;
    struct ASTNode *right;
} BinaryOpNode;

// Nó de operação unária
typedef struct {
    ASTNode base;
    Operator op;
    struct ASTNode *operand;
} UnaryOpNode;

// Funções para criar nós
ASTNode* create_program_node();
ASTNode* create_declaration_node(DataType type, char *name, ASTNode *init, int array_size);
ASTNode* create_assignment_node(char *var, Operator op, ASTNode *value);
ASTNode* create_expression_node(Operator op, ASTNode *left, ASTNode *right, char *value);
ASTNode* create_condition_node(Operator op, ASTNode *left, ASTNode *right);
ASTNode* create_if_node(ASTNode *condition, ASTNode *then_stmt, ASTNode *else_stmt);
ASTNode* create_while_node(ASTNode *condition, ASTNode *body);
ASTNode* create_for_node(ASTNode *init, ASTNode *condition, ASTNode *increment, ASTNode *body);
ASTNode* create_compound_node(ASTNode *statements);
ASTNode* create_return_node(ASTNode *value);
ASTNode* create_control_node(int is_break);
ASTNode* create_identifier_node(char *name);
ASTNode* create_number_node(char *value, DataType type);
ASTNode* create_char_literal_node(char *value);
ASTNode* create_string_literal_node(char *value);
ASTNode* create_binary_op_node(Operator op, ASTNode *left, ASTNode *right);
ASTNode* create_unary_op_node(Operator op, ASTNode *operand);

// Funções para manipular a AST
void add_statement(ASTNode *program, ASTNode *statement);
void free_ast(ASTNode *node);
void print_ast(ASTNode *node, int depth);

// Funções de otimização
void mark_dead_code(ASTNode *node);
void remove_dead_code(ASTNode *node);
void optimize_constant_folding(ASTNode *node);
void optimize_unreachable_code(ASTNode *node);

#endif // AST_H 