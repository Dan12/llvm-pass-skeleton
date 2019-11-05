int somefunc(int x) {
    int i = 0;
    int j = 0;
    int sum = 0;
    for (i = 0; i < x; i++) {
        sum += x/i;
    }
    for (j = 0; j < x; j+=2) {
        sum += x*j;
    }
    return sum;
}

int main() {
    int c = 0;
    c+= somefunc(10);
    c+= somefunc(-10);
    c+= somefunc(0);
    c+= somefunc(4);
    return c;
}