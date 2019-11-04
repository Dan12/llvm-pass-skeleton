int loopy(int x) {
    int i;
    int sum = 0;
    for (i = 0; i < x; i++) {
        if (i % 2 == 0) {
            sum += i;
        } else {
            sum *= i;
        }
    }
    return sum;
}

int main() {
  loopy(3);
  loopy(-1);
  return loopy(4);
}