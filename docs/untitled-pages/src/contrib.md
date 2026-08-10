---
title: Contribuindo
x-toc-enable: true
...

Como contribuir
===============

O TinyOpt é software livre (GPLv3). Contribuições de código, documentação, exemplos e testes são bem-vindas.

Passos
------

1. Clone ou faça fork de <https://github.com/leozamboni/tiny-opt>
2. Compile com `make` e valide com os exemplos em `examples/`
3. Implemente a alteração (preferencialmente em um branch)
4. Abra um pull request

Áreas úteis
-----------

- Novos passes de otimização em `opt/`
- Melhorias na análise de CFG / liveness
- Exemplos didáticos e testes
- Documentação em `docs/untitled-pages/src/`

Formatação
----------

```
make format
```

Diagrama UML (opcional)
-----------------------

```
make uml
```

Requer `clang-uml` e `plantuml`.
