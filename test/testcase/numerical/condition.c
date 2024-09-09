// checker=debug-inspection

void knight_dump_zval(int);
void knight_reachable();

void reachability(int x) {
    x = 2;
    if (x == 1) {
        knight_reachable();
        // warning:-1:9:-1:9: Unreachable [debug-inspection]
    } else {
        knight_reachable();
        // warning:-1:9:-1:9: Reachable [debug-inspection]
    }
}

void foo(int x) {
    x = 1;
    int b = x == 1;
    if (b) {
        knight_reachable();
        // warning:-1:9:-1:9: Reachable [debug-inspection]
        knight_dump_zval(x);
        // warning:-1:26:-1:26: 1 [debug-inspection]
    } else {
        knight_reachable();
        // warning:-1:9:-1:9: Unreachable [debug-inspection]
        int y = -x;
        knight_dump_zval(y);
        // warning:-1:26:-1:26: ⊥ [debug-inspection]
    }
}

void bar(int x) {
    if (x > 0) {
        knight_dump_zval(x);
        // warning:-1:26:-1:26: [1, +oo] [debug-inspection]
    } else {
        knight_dump_zval(x);
        // warning:-1:26:-1:26: [-oo, 0] [debug-inspection]
    }
}