#ifndef AST_H
#define AST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

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
    NODE_UNARY_OP,
    NODE_FUNCTION_DEF,
    NODE_FUNCTION_CALL,
    NODE_PARAMETER_LIST
} NodeType;

typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_CHAR,
    TYPE_VOID,
    TYPE_ARRAY
} DataType;

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

typedef struct ASTNode {
    NodeType type;
    struct ASTNode *next;  
    struct ASTNode *parent;
    int line;              
    int is_dead_code;      
} ASTNode;

typedef struct {
    ASTNode base;
    struct ASTNode *statements;
} ProgramNode;

typedef struct {
    ASTNode base;
    DataType data_type;
    char *name;
    struct ASTNode *initial_value;
    int array_size;
} DeclarationNode;

typedef struct {
    ASTNode base;
    char *variable;
    Operator op;
    struct ASTNode *value;
} AssignmentNode;

typedef struct {
    ASTNode base;
    Operator op;
    struct ASTNode *left;
    struct ASTNode *right;
    char *value;  
} ExpressionNode;

typedef struct {
    ASTNode base;
    Operator op;
    struct ASTNode *left;
    struct ASTNode *right;
} ConditionNode;

typedef struct {
    ASTNode base;
    struct ASTNode *condition;
    struct ASTNode *then_statement;
    struct ASTNode *else_statement;
} IfNode;

typedef struct {
    ASTNode base;
    struct ASTNode *condition;
    struct ASTNode *body;
    uint64_t loop_hash;
} WhileNode;

typedef struct {
    ASTNode base;
    struct ASTNode *init;
    struct ASTNode *condition;
    struct ASTNode *increment;
    struct ASTNode *body;
    uint64_t loop_hash;
} ForNode;

typedef struct {
    ASTNode base;
    struct ASTNode *statements;
} CompoundNode;

typedef struct {
    ASTNode base;
    struct ASTNode *value;
} ReturnNode;

typedef struct {
    ASTNode base;
    int is_break; 
} ControlNode;

typedef struct {
    ASTNode base;
    char *name;
} IdentifierNode;

typedef struct {
    ASTNode base;
    char *value;
    DataType data_type;
} NumberNode;

typedef struct {
    ASTNode base;
    char *value;
} CharLiteralNode;

typedef struct {
    ASTNode base;
    char *value;
} StringLiteralNode;

typedef struct {
    ASTNode base;
    Operator op;
    struct ASTNode *left;
    struct ASTNode *right;
} BinaryOpNode;

typedef struct {
    ASTNode base;
    Operator op;
    struct ASTNode *operand;
} UnaryOpNode;

typedef struct {
    ASTNode base;
    DataType return_type;
    char *name;
    struct ASTNode *parameters; 
    struct ASTNode *body;      
} FunctionDefNode;

typedef struct {
    ASTNode base;
    char *name;
    struct ASTNode *arguments;
} FunctionCallNode;

typedef struct {
    ASTNode base;
    struct ASTNode *parameters;
} ParameterListNode;

ASTNode* create_program_node();
ASTNode* create_declaration_node(DataType type, char *name, ASTNode *init, int array_size);
ASTNode* create_assignment_node(char *var, Operator op, ASTNode *value);
ASTNode* create_expression_node(Operator op, ASTNode *left, ASTNode *right, char *value);
ASTNode* create_condition_node(Operator op, ASTNode *left, ASTNode *right);
ASTNode* create_if_node(ASTNode *condition, ASTNode *then_stmt, ASTNode *else_stmt);
ASTNode* create_while_node(ASTNode *condition, ASTNode *body, uint64_t loop_hash);
ASTNode* create_for_node(ASTNode *init, ASTNode *condition, ASTNode *increment, ASTNode *body, uint64_t loop_hash);
ASTNode* create_compound_node(ASTNode *statements);
ASTNode* create_return_node(ASTNode *value);
ASTNode* create_control_node(int is_break);
ASTNode* create_identifier_node(char *name);
ASTNode* create_number_node(char *value, DataType type);
ASTNode* create_char_literal_node(char *value);
ASTNode* create_string_literal_node(char *value);
ASTNode* create_binary_op_node(Operator op, ASTNode *left, ASTNode *right);
ASTNode* create_unary_op_node(Operator op, ASTNode *operand);
ASTNode* create_function_def_node(DataType return_type, char *name, ASTNode *parameters, ASTNode *body);
ASTNode* create_function_call_node(char *name, ASTNode *arguments);
ASTNode* create_parameter_list_node(ASTNode *parameters);

void add_statement(ASTNode *program, ASTNode *statement);
ASTNode *add_args(ASTNode *list, ASTNode *expr);
void free_ast(ASTNode *node);
void print_ast(ASTNode *node, int depth);

#endif // AST_H 