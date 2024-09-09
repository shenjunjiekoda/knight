// checker=debug-inspection

#define NULL 0

// void clang_analyzer_dump(void*);
void clang_analyzer_dump(int);

void assign(int* x) {
    int a = 10;
    // clang_analyzer_dump(a);
    int* p = &a;
    // clang_analyzer_dump(p);
    int* q = p;
    // clang_analyzer_dump(q);
    int* r = NULL;
    // clang_analyzer_dump(r);
    int* s = x;
    // clang_analyzer_dump(s);
    // clang_analyzer_dump(*s);
}