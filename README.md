# Knight

Knight is a static analysis tool for c/c++ programs written in C++20.

# Install

knight dependencies:
- cmake
- llvm/clang(>=18 is best)

```SHELL
$ cmake -DLLVM_BUILD_DIR=/path/to/llvm/build -B build # use compiler support cpp20
$ cmake --build build -j$(nproc)
```

# Contributing

Contributions are welcome, See [CONTRIBUTING.md](CONTRIBUTING.md) for more details

# Licence

Knight is MIT-licensed, see [LICENSE](LICENSE) for more details
