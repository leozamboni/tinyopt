---
title: Documentação
x-toc-enable: true
...

Guia de início rápido
=====================

Dependências
------------

- GCC (C99)
- flex
- bison
- make

Compilação
----------

```
make
```

Isso gera o binário `tinyopt` na raiz do repositório.

Uso básico
----------

O TinyOpt lê o programa pela entrada padrão, aplica as otimizações e imprime o código otimizado na saída padrão:

```
./tinyopt < examples/factorial.c
```

Para emitir o grafo de fluxo de controle (CFG) em formato DOT (Graphviz):

```
./tinyopt --cfg < examples/factorial.c
```

Exemplos
--------

Há programas de demonstração em `examples/`:

| Arquivo | Foco |
|---------|------|
| `assig.c` | Atribuições |
| `conditionals.c` | Condicionais com folding |
| `loops.c` | Laços |
| `functions.c` | Funções |
| `factorial.c` | Dead code + recursão |
| `fibonacci.c` | Laços e liveness |
| `gcd.c` | Algoritmo clássico |
| `power.c` | Variáveis mortas |
| `prime.c` | Condicionais e laços |

Arquitetura
===========

Pipeline
--------

O fluxo de compilação em `tinyopt_compile` é:

1. **Análise léxica** (`lexer.l` / flex)
2. **Análise sintática** (`parser.y` / bison) → AST
3. **Otimização** (`tinyopt_optimize` em `tinyopt_core.c`)
4. **Emissão** do código otimizado ou do CFG em DOT

Estrutura de módulos
--------------------

```
tinyopt.c          — entrada, flags e orquestração
tinyopt_ast.*      — árvore sintática abstrata
tinyopt_stab.*     — tabela de símbolos
tinyopt_core.*     — pipeline de otimização e poda de dead code
tinyopt_cfg.*      — construção do CFG
tinyopt_dot_cfg.*  — emissão DOT do CFG
tinyopt_code.*     — pretty-print do código otimizado
opt/               — passes de otimização
```

Passes de otimização
--------------------

A ordem em `tinyopt_optimize` é:

1. **Constant folding** (`opt/tinyopt_folding.c`)  
   Avalia em tempo de compilação expressões e condições com operandos constantes.

2. **Tabela de símbolos** (`tinyopt_stab_init`)  
   Coleta declarações e valores conhecidos por escopo.

3. **Reachability** (`opt/tinyopt_reachability.c`)  
   Marca ramos inalcançáveis (ex.: `if (0)`, código após `return`).

4. **Liveness** (`opt/tinyopt_liveness.c`)  
   Análise de vivacidade sobre o CFG; marca atribuições e declarações mortas.

5. **Empty blocks** (`opt/tinyopt_empty_blocks.c`)  
   Marca blocos vazios ou compostos apenas por código morto.

6. **Remoção** (`tinyopt_remove_dead_code`)  
   Remove da AST os nós marcados com `is_dead_code`.

CFG
---

O CFG (`tinyopt_cfg.h`) modela o fluxo com nós ligados por `succ_true` / `succ_false`. É usado pela análise de liveness e pode ser visualizado com `--cfg` + Graphviz:

```
./tinyopt --cfg < examples/gcd.c | dot -Tpng -o cfg.png
```

Linguagem suportada
===================

O TinyOpt aceita um **subconjunto de C** com:

- Tipos: `int`, `float`, `char`, `void` (e arrays unidimensionais)
- Declarações, atribuições (`=`, `+=`, `-=`, `*=`, `/=`, `%=`, `++`, `--`)
- Condicionais (`if` / `else`)
- Laços (`while`, `for`)
- Funções (definição e chamada)
- `return`, `break`, `continue`
- Expressões aritméticas e relacionais / lógicas

A gramática completa está na página [EBNF](ebnf.md).

Perguntas frequentes
====================

O TinyOpt gera executáveis?
---------------------------

Não. Ele analisa e otimiza a AST e reemite código-fonte (ou o CFG). Não há geração de assembly ou ligação.

O código otimizado é semanticamente equivalente?
-----------------------------------------------

Sim, dentro do modelo adotado pelas análises (valores conhecidos estaticamente, pureza de expressões, etc.). O objetivo é didático: tornar as transformações observáveis no código.

Como contribuir?
----------------

Veja [Contribuindo](contrib.md) e o repositório no [GitHub](https://github.com/leozamboni/tiny-opt).

Qual é a licença?
-----------------

GPLv3. Detalhes em [Licença](license.md).
