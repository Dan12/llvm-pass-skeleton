int somefunc(int x) {
    int i = 0;
    int j = 0;
    int sum = 0;
    for (i = 0; i < x; i++) {
        for (j = 0; j < i; j++) {
            sum += i*j;
        }
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