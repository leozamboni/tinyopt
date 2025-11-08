int power(int base, int exp) {
    int result = 1;

    while (exp > 0) {
        if (exp % 2 == 1)
            result *= base;
        base *= base;
        exp /= 2;
    }

    return result;
}

int main() {
    int base = 2;
    int exp = 1;
    if (exp == 3) {
        exp = 4;
    }
    exp = 4;
    return power(base, exp);
    exp = 10;
}