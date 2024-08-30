// checker=debug*

void knight_dump_zval(int);

void foo() {
    int i = 0;
    while (i < 10) {
        i = i + 1;
        knight_dump_zval(i);
        // warning:-1:26:-1:26: [1, +oo] [debug-inspection]
    }
    knight_dump_zval(i);
    // warning:-1:22:-1:22: [10, +oo] [debug-inspection]
}