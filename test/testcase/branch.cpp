void knight_dump(int x);
void knight_assert(bool cond);

void if_branch(int a, int b) {
    int x = 1;
    if (b > 0) {
        x = a;
    } else {
        x = b;
    }
    knight_dump(x);
    if (b > 0) {
        knight_assert(b <= 0);
    }
}

void for_branch(int a, int b) {
    int x = 1;
    for (unsigned i = 0; i < 10; i++) {
        if (i == 5) {
            x = a;
            a++;
        } else {
            x = b;
            b--;
        }
        knight_dump(x);
    }
    knight_dump(x);
}

void for_loop(int n) {
    int arr[10];
    for (int i = 0; i < n; i++) {
        arr[i + 2] = i;
    }
}