# Tiny-Opt Compiler

Um compilador simples com sintaxe C-like que suporta declarações, atribuições e controle de fluxo.

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

## Exemplo de Código

```c
int x;
float y;
char c;

x = 5;
y = 3.14;
c = 'a';

x += 2;
y *= 2.0;
x++;

if (x > 3) {
    x = 10;
} else {
    x = 0;
}

while (x > 0) {
    x--;
}

if (x == 0 && y > 0) {
    break;
}

return x;
```

## Limpeza

Para limpar os arquivos gerados:
```bash
make clean
```

## Estrutura do Projeto

- `lexer.l` - Especificação do analisador léxico (Flex)
- `parser.y` - Especificação do analisador sintático (Bison)
- `main.c` - Programa principal
- `Makefile` - Script de compilação
- `test_simple.c` - Arquivo de teste
- `README.md` - Documentação

## Tecnologias Utilizadas

- **Flex** - Gerador de analisadores léxicos
- **Bison** - Gerador de analisadores sintáticos
- **GCC** - Compilador C
- **Make** - Sistema de build 