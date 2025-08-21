#include <stdio.h>
#include "ast.h"
#include "optimizer.h"

int yyparse(void);

// Variável global para armazenar a AST
ASTNode *global_ast = NULL;

int main() {
    printf("=== Tiny-Opt Compiler ===\n");
    printf("Digite o código C:\n");
    
    // Criar nó raiz do programa
    global_ast = create_program_node();
    
    // Fazer o parsing
    yyparse();
    
    // Aplicar otimizações
    if (global_ast) {
        optimize_code(global_ast);
    }
    
    // Limpar memória
    if (global_ast) {
        free_ast(global_ast);
    }
    
    return 0;
}
