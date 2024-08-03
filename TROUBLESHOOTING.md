# Troubleshooting

### linking error due to LLVM dynamic linking errors

Running Cmake with both `-DLINK_CLANG_DYLIB=ON` and `-DLINK_LLVM_DYLIB=ON` should fix the issue.