// checker=debug-inspection
// arg=-Xc
// arg=-analyze-with-threshold=true

void knight_dump_zval(int);

void foo(int i) {
    i = 0;
    while (i < 10) { // reg_$6<int i>
        // reg_$9<int i>
        knight_dump_zval(i);
        // warning:-1:26:-1:26: [0, 9] [debug-inspection]
        i = i + 1;
        // reg_$5<int i>
        knight_dump_zval(i);
        // warning:-1:26:-1:26: [1, 10] [debug-inspection]
    }
    knight_dump_zval(i);
    // warning:-1:22:-1:22: 10 [debug-inspection]
}

void bar() {
    for (int i = 0; i < 10; i++) {
        knight_dump_zval(i);
        // warning:-1:26:-1:26: [0, 9] [debug-inspection]
        for (int j = 0; j < 20; j++) {
            knight_dump_zval(i);
            // warning:-1:30:-1:30: [0, 9] [debug-inspection]
            knight_dump_zval(j);
            // warning:-1:30:-1:30: [0, 19] [debug-inspection]
        }
    }
}
