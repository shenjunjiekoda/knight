// checker=debug*

void knight_dump_zval(int);

void foo() {
    int i = 0;
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