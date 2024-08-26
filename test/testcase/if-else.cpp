// analysis=demo-itv-analysis
// checker=debug*

void knight_dump_zval(int);

int foo(int x) {
    int a = 0;
    knight_dump_zval(a);
    // warning:-1:22:-1:22: 0 [debug-inspection]
    if (x) {
        a = 1;
        knight_dump_zval(a);
        // warning:-1:26:-1:26: 1 [debug-inspection]
    } else {
        a = -1;
        knight_dump_zval(a);
        // warning:-1:26:-1:26: -1 [debug-inspection]
    }
    knight_dump_zval(a);
    // warning:-1:22:-1:22: [-1, 1] [debug-inspection]
    return a;
}

int bar(int x) {
    int a = 1;
    int b = 2;
    int c = x ? (a + b) : (a - b);
    knight_dump_zval(c);
    // warning:-1:22:-1:22: [-1, 3] [debug-inspection]
    return c;
}