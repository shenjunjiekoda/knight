// checker=debug*

void knight_dump_zval(int);

int foo(int x) {
    x = 2;
    x += 3;
    knight_dump_zval(x);
    // warning:-1:22:-1:22: 5 [debug-inspection]
    x *= 4;
    knight_dump_zval(x);
    // warning:-1:22:-1:22: 20 [debug-inspection]
    x /= 2;
    knight_dump_zval(x);
    // warning:-1:22:-1:22: 10 [debug-inspection]
    x -= 1;
    knight_dump_zval(x);
    // warning:-1:22:-1:22: 9 [debug-inspection]
    x <<= 1;
    knight_dump_zval(x);
    // warning:-1:22:-1:22: 18 [debug-inspection]
    x >>= 1;
    knight_dump_zval(x);
    // warning:-1:22:-1:22: 9 [debug-inspection]
    x = x % 2;
    knight_dump_zval(x);
    // warning:-1:22:-1:22: 1 [debug-inspection]
    return x;
}