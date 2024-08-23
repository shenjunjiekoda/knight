void knight_dump_zval(int);

int foo(int x) {
    int a = 0;
    if (x) {
        a = 1;
    } else {
        a = -1;
    }
    knight_dump_zval(a);
    return a;
}