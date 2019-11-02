int somefunc(int x) {
    int i = 0;
    if (x == 0) {
        i = 5;
    }
    if (x <= 0) {
        i += -12;
    } else {
        i += x*2;
    }
    i*=3;
    if (x == 5) {
        i += 4;
    }
    return i + 4;
}

int main() {
    int c = 0;
    c+= somefunc(10);
    c+= somefunc(-10);
    c+= somefunc(0);
    c+= somefunc(4);
    return c;
}