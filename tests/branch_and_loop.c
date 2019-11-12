int somefunc(int x) {
    int sum = 0;
  if (x >= 0) {
    for (int i = 0; i < x; i++) {
        if (i%2 == 0) {
            sum += i;
        } else {
            sum += 2*i;
        }
    }
  }
  if (x == 4) {
      sum -= 10;
  } else {
      sum += 12;
  }
  return x + sum;
}

int main() {
  somefunc(-4);
  somefunc(5);
  return somefunc(10);
}