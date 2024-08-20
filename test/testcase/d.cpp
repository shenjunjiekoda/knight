int foo(int x) {
    int a = -1;
    int b = 2;
    int c = a + b;
    int d = a + b + c;
    if (d > x) {
        d = 5;
    } else {
        x = 2;
        d = x;
    }
    return d + 1;
}