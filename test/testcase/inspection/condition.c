// checker=debug*

void knight_dump_zval(int);

void foo(int x) {
    x = 1;
    int b = x == 1;
    if (b) {
        knight_dump_zval(x);
        // warning:-1:26:-1:26: 1 [debug-inspection]
    } else {
        int y = -x;
        knight_dump_zval(y);
        // warning:-1:26:-1:26: -1 [debug-inspection]
    }
}