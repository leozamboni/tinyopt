#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cfg.h"

typedef struct CFGBuildResult {
	int entry_id;
	int exit_id;
} CFGBuildResult;

static int node_counter = 0;

static int new_node_id() {
	return ++node_counter;
}

static void emit_node_label(int id, const char *label) {
	printf("  n%d [label=\"%s\"];\n", id, label);
}

static void emit_edge(int from, int to, const char *label) {
	if (label && *label) {
		printf("  n%d -> n%d [label=\"%s\"];\n", from, to, label);
	} else {
		printf("  n%d -> n%d;\n", from, to);
	}
}

static void append_stmt_list_labels(ASTNode *node, char *buf, size_t bufsz) {
	for (ASTNode *cur = node; cur; cur = cur->next) {
		if (strlen(buf) > bufsz - 8) break;
		switch (cur->type) {
			case NODE_DECLARATION: strncat(buf, "decl; ", bufsz - strlen(buf) - 1); break;
			case NODE_ASSIGNMENT: strncat(buf, "assign; ", bufsz - strlen(buf) - 1); break;
			case NODE_RETURN: strncat(buf, "return; ", bufsz - strlen(buf) - 1); break;
			case NODE_BREAK: strncat(buf, "break; ", bufsz - strlen(buf) - 1); break;
			case NODE_CONTINUE: strncat(buf, "continue; ", bufsz - strlen(buf) - 1); break;
			default: strncat(buf, "stmt; ", bufsz - strlen(buf) - 1); break;
		}
	}
}

static const char* simple_stmt_label(ASTNode *node, char *tmp, size_t tmpsz) {
	if (!node) return "";
	switch (node->type) {
		case NODE_DECLARATION: {
			DeclarationNode *d = (DeclarationNode*)node;
			snprintf(tmp, tmpsz, "decl %s", d->name ? d->name : "");
			return tmp;
		}
		case NODE_ASSIGNMENT: {
			AssignmentNode *a = (AssignmentNode*)node;
			snprintf(tmp, tmpsz, "assign %s", a->variable ? a->variable : "");
			return tmp;
		}
		case NODE_RETURN:
			return "return";
		case NODE_BREAK:
			return "break";
		case NODE_CONTINUE:
			return "continue";
		default:
			return "stmt";
	}
}

static CFGBuildResult build_cfg_list(ASTNode *list);

static CFGBuildResult build_linear_stmt(ASTNode *stmt) {
	CFGBuildResult r = {0,0};
	int id = new_node_id();
	char tmp[128];
	const char *label = simple_stmt_label(stmt, tmp, sizeof(tmp));
	emit_node_label(id, label);
	r.entry_id = id;
	r.exit_id = id;
	return r;
}

static CFGBuildResult build_cfg_stmt(ASTNode *stmt) {
	CFGBuildResult r = {0,0};
	if (!stmt) return r;
	
	switch (stmt->type) {
		case NODE_IF_STATEMENT: {
			IfNode *i = (IfNode*)stmt;
			if (!i->condition && !i->then_statement && i->else_statement) {
				return build_cfg_stmt(i->else_statement);
			}
			if (!i->condition && i->then_statement && !i->else_statement) {
				return build_cfg_stmt(i->then_statement);
			}
			if (!i->then_statement && !i->else_statement) {
				CFGBuildResult empty = {0,0};
				return empty;
			}

			int cond = new_node_id();
			emit_node_label(cond, "if cond");
			CFGBuildResult then_r = {0,0};
			CFGBuildResult else_r = {0,0};
			if (i->then_statement) then_r = build_cfg_stmt(i->then_statement);
			if (i->else_statement) else_r = build_cfg_stmt(i->else_statement);
			int join = new_node_id();
			emit_node_label(join, "join");
			if (then_r.entry_id) emit_edge(cond, then_r.entry_id, "true");
			else emit_edge(cond, join, "true");
			if (else_r.entry_id) emit_edge(cond, else_r.entry_id, "false");
			else emit_edge(cond, join, "false");
			if (then_r.exit_id) emit_edge(then_r.exit_id, join, NULL);
			if (else_r.exit_id) emit_edge(else_r.exit_id, join, NULL);
			r.entry_id = cond;
			r.exit_id = join;
			return r;
		}
		case NODE_WHILE_STATEMENT: {
			WhileNode *w = (WhileNode*)stmt;
			int cond = new_node_id();
			emit_node_label(cond, "while cond");
			CFGBuildResult body_r = {0,0};
			if (w->body) body_r = build_cfg_stmt(w->body);
			int after = new_node_id();
			emit_node_label(after, "after while");
			if (body_r.entry_id) emit_edge(cond, body_r.entry_id, "true");
			else emit_edge(cond, cond, "true");
			if (body_r.exit_id) emit_edge(body_r.exit_id, cond, NULL);
			emit_edge(cond, after, "false");
			r.entry_id = cond;
			r.exit_id = after;
			return r;
		}
		case NODE_FOR_STATEMENT: {
			ForNode *f = (ForNode*)stmt;
			CFGBuildResult init_r = {0,0};
			if (f->init) init_r = build_cfg_stmt(f->init);
			int cond = new_node_id();
			emit_node_label(cond, "for cond");
			CFGBuildResult body_r = {0,0};
			if (f->body) body_r = build_cfg_stmt(f->body);
			CFGBuildResult inc_r = {0,0};
			if (f->increment) inc_r = build_cfg_stmt(f->increment);
			int after = new_node_id();
			emit_node_label(after, "after for");
			// encadeamento
			if (init_r.exit_id) emit_edge(init_r.exit_id, cond, NULL);
			else if (init_r.entry_id) emit_edge(init_r.entry_id, cond, NULL);
			if (body_r.entry_id) emit_edge(cond, body_r.entry_id, "true");
			else emit_edge(cond, cond, "true");
			if (body_r.exit_id) {
				if (inc_r.entry_id) {
					emit_edge(body_r.exit_id, inc_r.entry_id, NULL);
					if (inc_r.exit_id) emit_edge(inc_r.exit_id, cond, NULL);
					else emit_edge(inc_r.entry_id, cond, NULL);
				} else {
					emit_edge(body_r.exit_id, cond, NULL);
				}
			}
			emit_edge(cond, after, "false");
			r.entry_id = init_r.entry_id ? init_r.entry_id : cond;
			r.exit_id = after;
			return r;
		}
		case NODE_COMPOUND_STATEMENT: {
			CompoundNode *c = (CompoundNode*)stmt;
			return build_cfg_list(c->statements);
		}
		default:
			return build_linear_stmt(stmt);
	}
}

static CFGBuildResult build_cfg_list(ASTNode *list) {
	CFGBuildResult r = {0,0};
	int prev_exit = 0;
	for (ASTNode *cur = list; cur; cur = cur->next) {
		CFGBuildResult cr = build_cfg_stmt(cur);
		if (!r.entry_id) r.entry_id = cr.entry_id;
		if (prev_exit && cr.entry_id) emit_edge(prev_exit, cr.entry_id, NULL);
		prev_exit = cr.exit_id ? cr.exit_id : prev_exit;
		r.exit_id = prev_exit;
	}
	return r;
}

void print_cfg_dot(ASTNode *ast) {
	node_counter = 0;
	printf("digraph CFG {\n");
	printf("  node [shape=box];\n");
	if (!ast) {
        printf("}\n");
        return;
    }
	if (ast->type == NODE_PROGRAM) {
		ProgramNode *p = (ProgramNode*)ast;
		CFGBuildResult r = build_cfg_list(p->statements);
		(void)r;
	} else {
		CFGBuildResult r = build_cfg_stmt(ast);
		(void)r;
	}
	printf("}\n");
}


