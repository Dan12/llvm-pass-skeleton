int somefunc(int x) {
    int i = 0;
    int sum = 0;
    for (i = 0; i < x; i++) {
        if (i == 4) {
            break;
        }
        sum += i;
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