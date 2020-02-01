# C/C++ API Fuzzing

> To be updated.

## Build

Before building this project, you need to build LLVM-7 from source. For instructions on building LLVM, please refer to [here](http://releases.llvm.org/7.0.0/docs/CMake.html).

To build this project, you need a `cmake` installation available. The recommended minimum version of `cmake` is 3.10.

### Steps to Build

Clone this repository:

```shell
git clone https://github.com/Lancern/caf.git
cd caf
```

Then create a build directory at the root directory of this repository:

```shell
mkdir build
cd build
```

Then execute the following command to configure the project using `cmake`:

```shell
cmake -DLLVM_BUILD_DIR=/path/to/llvm/build/tree ..
```

Then you can build the project using the following commands:

```shell
cmake --build .
```

### CMake Options Available

Some CMake options are available when configuring and generating build scripts. They can be passed to cmake using the `-D` flag, in the form `-D<name>[=<value>]`, e.g. `-DLLVM_BUILD_DIR=...`. The following table lists all available options:

| Name | Required | Notes |
|------|----------|-------|
| `LLVM_BUILD_DIR` | Yes | Path to the root directory of the LLVM build tree |

## Engineering

### Documents

All available documents are maintained under `docs/`. Doxygen documents are also available, to
generate a copy of doxygen documents, execute the following command in the root directory of this
project:

```shell
doxygen
```

Then doxygen documents will be available under `docs/doxygen`.

### Editor Plugin

It's recommended to use `clangd` while developing. To prepare a `compile_commands.json` file, which is essential for `clangd`, at the root directory of this project, execute the following commands at the root directory of this project:

```shell
mkdir build && cd build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 [Other CMake Options] ..
cp compile_commands.json ../
```

`[Other CMake Options]` are those options available during regular build process.

For more information on using `clangd`, please refer to [here](https://clang.llvm.org/extra/clangd/).
