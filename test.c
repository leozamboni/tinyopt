int x;
float y;
char c;
int i;
int arr[10];

x = 5;
y = 3.14;
c = 'a';
i = 0;
x += 2;
y *= 2.0;
x++;
--x;

if (x > 3) {
    x = 10;
} else {
    x = 0;
}

while (x > 0) {
    x--;
}

for (i = 0; i < 5; i++) {
    i = i * 2;
}

if (x == 0 && y > 0) {
    break;
}

if (x != 0 || y < 0) {
    continue;
}

return x; 