int add4(int x) {
  if (x <= 0) {
    return 0;
  }
  return x + 4;
}

int main() {
  add4(-4);
  add4(5);
  return add4(10);
}