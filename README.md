CppBundler
===

A tool to bundle custom headers into one single `.cpp` file.

## Dependencies

This tool is based on LLVM/Clang, so please install them first.

If you are using Ubuntu, you could install these 2 packages:

```bash
$ apt install llvm-dev libclang-dev
```

## Build

```bash
$ mkdir build && cd build
$ cmake .. && cmake --build .
$ ./cpp-bundle FILE [OPTIONS]...
```

## Options

Since this tool is based on LLVM/Clang preprocessor, you could use the same options to bundle your code.

## Example Use Case

It is common to use a library in online competitive programming contests like Codeforces, AtCoder, etc. However, this kind of contest only allows you to submit a single C++ code. Therefore, if we modularize our library, we need to bundle them.
