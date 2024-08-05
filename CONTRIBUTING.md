# Contribution Guidelines

## Reporting Issues

If you encounter a problem when using infer or if you have any questions, please open a GitHub issue.

## Hacking on the Code

We welcome contributions via [pull requests](https://github.com/shenjunjiekoda/knight/pulls) on GitHub.

## Coding Style

Please run format tools before committing your changes. We use [clang-format](https://clang.llvm.org/docs/ClangFormat.html) for C++ code.

```shell
$ chmod +x scripts/run-clang-format
$ scripts/run-clang-format # --help for usages
```

```shell
$ chmod +x scripts/run-clang-tidy
$ scripts/run-clang-tidy -j $(nproc) # --help for usages
```