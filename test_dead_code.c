int x;
int y;
int z;
int unused_var;

x = 5;
y = 10;
z = 15;
unused_var = 20;  // Variável não utilizada

if (0) {
    x = 100;  // Código morto - nunca executado
} else {
    y = 200;
}

if (1) {
    z = 300;
} else {
    x = 400;  // Código morto - nunca executado
}

while (0) {
    y = 500;  // Código morto - loop nunca executado
}

return x; 