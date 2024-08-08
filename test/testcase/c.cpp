void clang_analyzer_dump(int) {}
void clang_analyzer_dump(void*) {}

// clang-format off
void foo(int a, int* b, int** c, void* d, char *e) {
    clang_analyzer_dump(a); // reg_$0<int a>
    clang_analyzer_dump(b); // &SymRegion{reg_$1<int * b>}
    clang_analyzer_dump(*b); // reg_$2<int Element{SymRegion{reg_$1<int * b>},0 S64b,int}>
    clang_analyzer_dump(c); // &SymRegion{reg_$3<int ** c>}
    clang_analyzer_dump(*c); // &SymRegion{reg_$4<int * Element{SymRegion{reg_$3<int ** c>},0 S64b,int *}>}
    clang_analyzer_dump(**c); // reg_$5<int Element{SymRegion{reg_$4<int * Element{SymRegion{reg_$3<int ** c>},0 S64b,int *}>},0 S64b,int}> [clang-analyzer-feysh.debug.ExprInspection]
    clang_analyzer_dump(d); // &SymRegion{reg_$6<void * d>}
    clang_analyzer_dump(*(int*)d); // reg_$7<int Element{SymRegion{reg_$6<void * d>},0 S64b,int}>
    clang_analyzer_dump(e); // &SymRegion{reg_$8<char * e>}
    clang_analyzer_dump(*e); // reg_$9<char Element{SymRegion{reg_$8<char * e>},0 S64b,char}>
    clang_analyzer_dump(*(int*)e); // reg_$10<int Element{SymRegion{reg_$8<char * e>},0 S64b,int}>
}
// clang-format on