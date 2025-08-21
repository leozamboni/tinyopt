# Tiny-Opt Compiler

Um compilador simples com sintaxe C-like que suporta declarações, atribuições, controle de fluxo e **otimização de código** para remoção de código morto.

## Funcionalidades Implementadas

### Tipos de Dados
- `int` - Números inteiros
- `float` - Números de ponto flutuante
- `char` - Caracteres
- `void` - Tipo vazio
- Arrays (ex: `int arr[10]`)

### Declarações
```c
int x;
float y;
char c;
int arr[10];
```

### Atribuições
- Atribuição simples: `x = 5;`
- Operadores compostos:
  - `+=` (adição)
  - `-=` (subtração)
  - `*=` (multiplicação)
  - `/=` (divisão)
  - `%=` (módulo)
- Incremento/Decremento:
  - `x++` (pós-incremento)
  - `++x` (pré-incremento)
  - `x--` (pós-decremento)
  - `--x` (pré-decremento)

### Operadores Aritméticos
- `+` (adição)
- `-` (subtração)
- `*` (multiplicação)
- `/` (divisão)
- `%` (módulo)

### Operadores de Comparação
- `==` (igual)
- `!=` (diferente)
- `<` (menor)
- `<=` (menor ou igual)
- `>` (maior)
- `>=` (maior ou igual)

### Operadores Lógicos
- `&&` (AND lógico)
- `||` (OR lógico)
- `!` (NOT lógico)

### Controle de Fluxo

#### Estruturas Condicionais
```c
if (x > 3) {
    x = 10;
} else {
    x = 0;
}
```

#### Loops
```c
while (x > 0) {
    x--;
}

for (int i = 0; i < 5; i++) {
    // código
}
```

#### Controle de Loop
- `break;` - Sai do loop
- `continue;` - Continua para a próxima iteração

### Literais
- Números: `5`, `3.14`
- Caracteres: `'a'`
- Strings: `"hello"`

### Estruturas de Bloco
```c
{
    // múltiplas declarações e instruções
    int x = 5;
    x++;
}
```

## 🚀 **Otimizador de Código**

O compilador inclui um **otimizador avançado** que detecta e marca código morto:

### Funcionalidades do Otimizador

#### 1. **Detecção de Código Morto**
- **Variáveis não utilizadas**: Detecta declarações e atribuições para variáveis que nunca são lidas
- **Código inalcançável**: Identifica código após `return`, `break`, `continue`
- **Condições sempre falsas**: Marca código em `if (0)` e `while (0)` como morto
- **Condições sempre verdadeiras**: Identifica ramos `else` em `if (1)`

#### 2. **Análise de Fluxo de Dados**
- **Tabela de variáveis**: Rastreia definição e uso de variáveis
- **Análise de escopo**: Identifica variáveis locais não utilizadas
- **Detecção de atribuições mortas**: Encontra atribuições que nunca são lidas

#### 3. **Otimizações Implementadas**
```c
// Exemplo de código com otimizações aplicadas
int x;
int y;
int unused_var;  // [DEAD] - Variável não utilizada

x = 5;
y = 10;
unused_var = 20;  // [DEAD] - Atribuição morta

if (0) {
    x = 100;  // [DEAD] - Código nunca executado
} else {
    y = 200;
}

while (0) {
    y = 500;  // [DEAD] - Loop nunca executado
}

return x;
// [DEAD] - Código após return é inalcançável
```

### Como o Otimizador Funciona

1. **Análise Léxica e Sintática**: Constrói uma árvore sintática abstrata (AST)
2. **Análise de Uso de Variáveis**: Rastreia definições e usos de variáveis
3. **Marcação de Código Morto**: Identifica e marca nós da AST como código morto
4. **Relatório de Otimização**: Exibe a AST com código morto marcado

### Estrutura do Otimizador

```
optimizer/
├── ast.h          # Definições da árvore sintática abstrata
├── ast.c          # Implementação da AST
├── optimizer.h    # Interface do otimizador
└── optimizer.c    # Implementação das otimizações
```

## Como Usar

1. Compile o projeto:
```bash
make
```

2. Execute o compilador:
```bash
./comp
```

3. Digite o código C ou use redirecionamento:
```bash
./comp < arquivo.c
```

## Exemplo de Código com Otimização

```c
int x;
int y;
int unused_var;

x = 5;
y = 10;
unused_var = 20;  // Será marcado como código morto

if (0) {
    x = 100;  // Será marcado como código morto
} else {
    y = 200;
}

while (0) {
    y = 500;  // Será marcado como código morto
}

return x;
```

**Saída do otimizador:**
```
=== Iniciando Otimização de Código ===
Variável não utilizada marcada como código morto: unused_var
Atribuição para variável não utilizada marcada como código morto: unused_var
=== Relatório de Otimização ===
Árvore Sintática Abstrata após otimização:
Program
  Declaration: x
  Declaration: y
  [DEAD] Declaration: unused_var
  Assignment: x = 5
  Assignment: y = 10
  [DEAD] Assignment: unused_var = 20
  If Statement
    [DEAD] Assignment: x = 100
    Assignment: y = 200
  [DEAD] While Statement
    [DEAD] Assignment: y = 500
  Return: x
=== Otimização Concluída ===
```

## Limpeza

Para limpar os arquivos gerados:
```bash
make clean
```

## Estrutura do Projeto

- `lexer.l` - Especificação do analisador léxico (Flex)
- `parser.y` - Especificação do analisador sintático (Bison)
- `ast.h` / `ast.c` - Árvore sintática abstrata
- `optimizer.h` / `optimizer.c` - Otimizador de código
- `main.c` - Programa principal
- `Makefile` - Script de compilação
- `test_dead_code.c` - Arquivo de teste com código morto
- `README.md` - Documentação

## Tecnologias Utilizadas

- **Flex** - Gerador de analisadores léxicos
- **Bison** - Gerador de analisadores sintáticos
- **GCC** - Compilador C
- **Make** - Sistema de build
- **AST** - Árvore sintática abstrata para análise
- **Análise de Fluxo de Dados** - Para detecção de código morto

## Próximos Passos

O otimizador pode ser expandido com:
- **Dobramento de constantes**: `2 + 3` → `5`
- **Eliminação de código redundante**: `x = 5; x = 10;` → `x = 10;`
- **Otimização de loops**: Unrolling, hoisting
- **Análise interprocedural**: Para funções
- **Geração de código otimizado**: Código assembly otimizado 