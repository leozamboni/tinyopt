---
title: Extended Backus–Naur form
x-toc-enable: true
...

program ::= { statement } ;

statement ::=
      declaration ";"                 -- declaração de variável/função (assinatura)
    | assignment ";"                  -- atribuição
    | function_call ";"               -- chamada de função
    | if_statement
    | while_statement
    | for_statement
    | compound_statement              -- bloco { ... }
    | function_def                    -- definição de função
    | "return" [ expression ] ";"     -- return opcionalmente com expressão
    | "break" ";"                     -- break
    | "continue" ";"                  -- continue
    ;

declaration ::=
      "int" <identifier> [ "[" <number> "]" ]
    | "float" <identifier> [ "[" <number> "]" ]
    | "char" <identifier> [ "[" <number> "]" ]
    | "void" <identifier> [ "[" <number> "]" ]
    ;   -- (declaração de parâmetro/variável simplificada conforme parser)

assignment ::=
      <identifier> "=" expression
    | <identifier> "+=" expression
    | <identifier> "-=" expression
    | <identifier> "*=" expression
    | <identifier> "/=" expression
    | <identifier> "%=" expression
    | <identifier> "++"
    | <identifier> "--"
    | "++" <identifier>
    | "--" <identifier>
    ;

if_statement ::=
      "if" "(" condition ")" statement
    | "if" "(" condition ")" statement "else" statement
    ;

while_statement ::=
      "while" "(" condition ")" statement
    ;

for_statement ::=
      "for" "(" [ declaration | assignment ] ";" [ condition ] ";" [ assignment ] ")" statement
    ; -- cobre as variações aceitas pelo parser (vazio/omissão de partes)

compound_statement ::=
    "{" compound_program "}"
    ;

compound_program ::= { statement } ;

condition ::=
    expression         -- expressão simples como condição
  | "!" condition
  | "(" condition ")"
  | expression relop expression
  ;

relop ::= "==" | "!=" | "<" | ">" | "<=" | ">=" | "&&" | "||" ;

expression ::=
    additive_expression
    ;

additive_expression ::=
    multiplicative_expression { ( "+" | "-" ) multiplicative_expression }
    ;

multiplicative_expression ::=
    unary_expression { ( "*" | "/" | "%" ) unary_expression }
    ;

unary_expression ::=
      [ ( "+" | "-" | "!" ) ] primary_expression
    ;

primary_expression ::=
      <number>
    | <char_literal>
    | <string_literal>
    | <identifier>
    | <identifier> "[" expression "]"
    | function_call
    | "(" expression ")"
    ;

function_call ::=
      <identifier> "(" [ argument_list ] ")"
    ;

argument_list ::=
      expression { "," expression }
    ;

function_def ::=
      ("int" | "float" | "char" | "void") <identifier> "(" [ parameter_list ] ")" compound_statement
    ;

parameter_list ::=
      declaration { "," declaration }
    ;

