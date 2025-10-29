#include <stdio.h>
#include "codegen.h"

static void emit_stmt_list(ASTNode *node);
static void emit_stmt(ASTNode *node);
static void emit_expr(ASTNode *node);
static const char* dtype_to_str(DataType t);
static const char* op_to_assign_str(Operator op);
static const char* relop_to_str(Operator op);
static void emit_indent(int n);
static void emit_block(ASTNode *node, int indent);

void print_optimized_code(ASTNode *ast) {
    printf("\n=== Código Otimizado (C-like) ===\n");
    if (!ast) { printf("\n"); return; }
    if (ast->type == NODE_PROGRAM) {
        ProgramNode *p = (ProgramNode*)ast;
        emit_stmt_list(p->statements);
    } else {
        emit_stmt(ast);
    }
    printf("\n");
}

static void emit_stmt_list(ASTNode *node) {
    for (ASTNode *cur = node; cur; cur = cur->next) {
        emit_stmt(cur);
    }
}

static const char* dtype_to_str(DataType t) {
    switch (t) {
        case TYPE_INT: return "int";
        case TYPE_FLOAT: return "float";
        case TYPE_CHAR: return "char";
        case TYPE_VOID: return "void";
        case TYPE_ARRAY: return "int"; // simplified
        default: return "int";
    }
}

static const char* op_to_assign_str(Operator op) {
    switch (op) {
        case OP_ASSIGN: return "=";
        case OP_ADD_ASSIGN: return "+=";
        case OP_SUB_ASSIGN: return "-=";
        case OP_MUL_ASSIGN: return "*=";
        case OP_DIV_ASSIGN: return "/=";
        case OP_MOD_ASSIGN: return "%=";
        default: return NULL;
    }
}

static const char* relop_to_str(Operator op) {
    switch (op) {
        case OP_EQ: return "==";
        case OP_NE: return "!=";
        case OP_LT: return "<";
        case OP_LE: return "<=";
        case OP_GT: return ">";
        case OP_GE: return ">=";
        default: return "?";
    }
}

static void emit_indent(int n) {
    for (int i = 0; i < n; i++) printf("  ");
}

static void emit_block(ASTNode *node, int indent) {
    printf("{\n");
    for (ASTNode *cur = node; cur; cur = cur->next) {
        emit_indent(indent);
        emit_stmt(cur);
    }
    emit_indent(indent-1);
    printf("}\n");
}

static void emit_stmt(ASTNode *node) {
    if (!node) return;
    switch (node->type) {
        case NODE_DECLARATION: {
            DeclarationNode *d = (DeclarationNode*)node;
            printf("%s %s", dtype_to_str(d->data_type), d->name);
            if (d->array_size > 0) printf("[%d]", d->array_size);
            if (d->initial_value) { printf(" = "); emit_expr(d->initial_value); }
            printf(";\n");
            break;
        }
        case NODE_ASSIGNMENT: {
            AssignmentNode *a = (AssignmentNode*)node;
            const char *as = op_to_assign_str(a->op);
            if (as) {
                printf("%s %s ", a->variable, as);
                if (a->value) emit_expr(a->value);
                printf(";\n");
            } else if (a->op == OP_INC) {
                printf("%s++;\n", a->variable);
            } else if (a->op == OP_DEC) {
                printf("%s--;\n", a->variable);
            } else {
                printf("/* unsupported assignment op */\n");
            }
            break;
        }
        case NODE_IF_STATEMENT: {
            IfNode *i = (IfNode*)node;

            if (i->then_statement && i->then_statement->type == NODE_COMPOUND_STATEMENT) {
                printf("if (");
                emit_expr(i->condition);
                printf(") ");
                emit_block(((CompoundNode*)i->then_statement)->statements, 1);
            }
            if (i->else_statement) {
                if (i->then_statement) {
                    printf("else ");
                }
                if (i->else_statement->type == NODE_COMPOUND_STATEMENT) {
                    emit_block(((CompoundNode*)i->else_statement)->statements, 1);
                } else {
                    printf("{\n  ");
                    emit_stmt(i->else_statement);
                    printf("}\n");
                }
            }
            break;
        }
        case NODE_WHILE_STATEMENT: {
            WhileNode *w = (WhileNode*)node;
            printf("while (");
            emit_expr(w->condition);
            printf(") ");
            if (w->body && w->body->type == NODE_COMPOUND_STATEMENT) {
                emit_block(((CompoundNode*)w->body)->statements, 1);
            } else {
                printf("{\n  ");
                emit_stmt(w->body);
                printf("}\n");
            }
            break;
        }
        case NODE_FOR_STATEMENT: {
            ForNode *f = (ForNode*)node;
            printf("for (");
            if (f->init && f->init->type == NODE_ASSIGNMENT) {
                AssignmentNode *a = (AssignmentNode*)f->init;
                printf("%s %s ", a->variable, op_to_assign_str(a->op));
                if (a->value) emit_expr(a->value);
            } else if (f->init && f->init->type == NODE_DECLARATION) {
                DeclarationNode *d = (DeclarationNode*)f->init;
                printf("%s %s", dtype_to_str(d->data_type), d->name);
                if (d->initial_value) { printf(" = "); emit_expr(d->initial_value); }
            }
            printf("; ");
            if (f->condition) emit_expr(f->condition);
            printf("; ");
            if (f->increment && f->increment->type == NODE_ASSIGNMENT) {
                AssignmentNode *a = (AssignmentNode*)f->increment;
                const char *as = op_to_assign_str(a->op);
                if (as) {
                    printf("%s %s ", a->variable, as);
                    if (a->value) emit_expr(a->value);
                } else if (a->op == OP_INC) {
                    printf("%s++", a->variable);
                } else if (a->op == OP_DEC) {
                    printf("%s--", a->variable);
                }
            }
            printf(") ");
            if (f->body && f->body->type == NODE_COMPOUND_STATEMENT) {
                emit_block(((CompoundNode*)f->body)->statements, 1);
            } else {
                printf("{\n  ");
                emit_stmt(f->body);
                printf("}\n");
            }
            break;
        }
        case NODE_COMPOUND_STATEMENT: {
            CompoundNode *c = (CompoundNode*)node;
            emit_block(c->statements, 1);
            break;
        }
        case NODE_RETURN: {
            ReturnNode *r = (ReturnNode*)node;
            printf("return ");
            emit_expr(r->value);
            printf(";\n");
            break;
        }
        case NODE_BREAK: {
            printf("break;\n");
            break;
        }
        case NODE_CONTINUE: {
            printf("continue;\n");
            break;
        }
        default:
            break;
    }
}

static void emit_expr(ASTNode *node) {
    if (!node) { printf(""); return; }
    switch (node->type) {
        case NODE_NUMBER:
            printf("%s", ((NumberNode*)node)->value);
            break;
        case NODE_IDENTIFIER:
            printf("%s", ((IdentifierNode*)node)->name);
            break;
        case NODE_BINARY_OP: {
            BinaryOpNode *b = (BinaryOpNode*)node;
            printf("(");
            emit_expr(b->left);
            const char *op = "?";
            switch (b->op) {
                case OP_ADD: op = "+"; break;
                case OP_SUB: op = "-"; break;
                case OP_MUL: op = "*"; break;
                case OP_DIV: op = "/"; break;
                case OP_MOD: op = "%"; break;
                default: break;
            }
            printf(" %s ", op);
            emit_expr(b->right);
            printf(")");
            break;
        }
        case NODE_CONDITION: {
            ConditionNode *c = (ConditionNode*)node;
            printf("(");
            emit_expr(c->left);
            printf(" %s ", relop_to_str(c->op));
            emit_expr(c->right);
            printf(")");
            break;
        }
        case NODE_UNARY_OP: {
            UnaryOpNode *u = (UnaryOpNode*)node;
            if (u->op == OP_NOT) {
                printf("!");
                emit_expr(u->operand);
            } else {
                emit_expr(u->operand);
            }
            break;
        }
        default:
            printf("0");
            break;
    }
}

