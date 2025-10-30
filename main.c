#include <stdio.h>
#include "ast.h"
#include "optimizer.h"
#include "cfg.h"
#include "codegen.h"

int yyparse(void);

ASTNode *global_ast = NULL;

int main(int argc, char **argv) {
    global_ast = create_program_node();
    
    yyparse();

	int emit_cfg = 0;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--cfg") == 0) emit_cfg = 1;
	}

	if (global_ast) {
		optimize_code(global_ast);
		if (emit_cfg) {
			print_cfg_dot(global_ast);
		} else {
			print_optimized_code(global_ast);
		}
	}

    if (global_ast) {
        free_ast(global_ast);
    }
    
    return 0;
}
