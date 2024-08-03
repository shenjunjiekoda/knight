<img src="doc/icon.png" alt="logo" width="20%" />

# Knight ![Language](https://img.shields.io/badge/language-c++-brightgreen)![License](https://img.shields.io/badge/license-MIT-yellow)

Knight is a static analysis tool for c/c++ programs written in C++20.

# Install

knight dependencies:
- cmake
- llvm/clang(>=18 is best)

You can install llvm by your package manager 
```SHELL
$ apt install llvm # for Ubuntu
$ brew install llvm # for Homebrew
```
or [build from source](https://llvm.org/docs/GettingStarted.html).

```SHELL
$ cmake -DLLVM_BUILD_DIR=/path/to/llvm/build -B build 
$ cmake --build build -j$(nproc)
```
Note: Knight only support C++20, so you need to use a compiler support C++20.

# Troubleshooting

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for more details.

# Contributing

Contributions are welcome, See [CONTRIBUTING.md](CONTRIBUTING.md) for more details

# Licence

Knight is MIT-licensed, see [LICENSE](LICENSE) for more details
