CppBundler
===

A tool to bundle custom headers into one single `.cpp` file.

## Dependencies

This tool is based on LLVM/Clang. You can either use system libraries or build strictly against a custom LLVM version.

### Option 1: System Dependencies (Dynamic Build)
On Ubuntu/Debian:
```bash
$ sudo apt install llvm-dev libclang-dev build-essential cmake ninja-build
```

### Option 2: Custom LLVM (Dynamic & Static Build)
For a fully static binary, you need to build LLVM from source or use a static distribution.
```bash
# Example: Build LLVM 21.1.0 from source
git clone https://github.com/llvm/llvm-project.git
cd llvm-project
git checkout llvmorg-21.1.0 
cmake -S llvm -B build -G Ninja \
    -DCMAKE_INSTALL_PREFIX=$HOME/llvm-install \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_ENABLE_PROJECTS="clang" \
    -DLLVM_ENABLE_ZSTD=OFF \
    -DBUILD_SHARED_LIBS=OFF
cmake --build build --target install
```

## Build

### Dynamic Build
**System Dependencies:**
```bash
cmake -S . -B build -G Ninja
cmake --build build
```

**Custom LLVM:**
If you have a custom LLVM installed (e.g. in `~/llvm-install`) but want to link dynamically:
```bash
cmake -S . -B build -G Ninja \
    -DLLVM_DIR=$HOME/llvm-install/lib/cmake/llvm \
    -DClang_DIR=$HOME/llvm-install/lib/cmake/clang

cmake --build build
```

### Static Build
If you built LLVM manually as above:
```bash
cmake -S . -B build -G Ninja \
    -DBUILD_STATIC=ON \
    -DUSE_STATIC_CRT=ON \
    -DLLVM_DIR=$HOME/llvm-install/lib/cmake/llvm \
    -DClang_DIR=$HOME/llvm-install/lib/cmake/clang

cmake --build build
```

## Usage

```bash
$ ./build/cpp-bundle FILE [OPTIONS]...
```

## Options

Since this tool is based on LLVM/Clang preprocessor, you could use the same options to bundle your code.

## Example Use Case

It is common to use a library in online competitive programming contests like Codeforces, AtCoder, etc. However, this kind of contest only allows you to submit a single C++ code. Therefore, if we modularize our library, we need to bundle them.
