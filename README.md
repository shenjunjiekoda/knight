<img src="doc/icon.png" alt="logo" width="20%" />

# Knight ![Language](https://img.shields.io/badge/language-c++-brightgreen)![License](https://img.shields.io/badge/license-MIT-yellow)

_Knight_ is a static analysis tool for C/C++ programs written in C++20.

# Install

## knight dependencies

- cmake
- llvm/clang(>=18 is best)
- GMP (>= 6 is best)

You can install llvm by your package manager

```SHELL
apt install llvm libgmp-dev libedit-dev # for Ubuntu
brew install llvm gmp # for Homebrew
yum install llvm gmp-devel libedit-devel # for CentOS
sudo pacman -S llvm gmp # for Arch Linux
```

or [build from source](https://llvm.org/docs/GettingStarted.html).

## Install knight by following steps

```SHELL
cd knight
chmod +x scripts/install
# scripts/install --help
scripts/install
```

or run step by step:

```SHELL
cd knight
cmake -DLLVM_BUILD_DIR=/path/to/llvm/build -B path-to-build -S .
cmake --build path-to-build -j$(nproc)
path-to-build/bin/knight --help # for usages
```

Note: Knight only support _C++20_, so you need to use a cpp compiler support C++20.

# Troubleshooting

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for more details.

# Contributing

Contributions are welcome, See [CONTRIBUTING.md](CONTRIBUTING.md) for more details

# Licence

Knight is MIT-licensed, see [LICENSE](LICENSE) for more details
