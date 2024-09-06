# Troubleshooting

### linking error due to LLVM dynamic linking errors

```shell
CommandLine Error: Option 'experimental-debuginfo-iterators' registered more than once!
LLVM ERROR: inconsistency in registered CommandLine options
[1]    26042 abort
```

When you encounter this error, it means you shall link against the LLVM dynamic library dynamic library.

Running cmake with `-DLINK_LLVM_DYLIB=ON` should fix the issue.
