# C/C++ API Fuzzing

> To be updated.

## Build

### Steps to Build

This project is configured to be built with `cmake`. Create a build directory
in the project first:

```shell
mkdir build
cd build
```

Then execute the following command to configure the project using `cmake`:

```shell
cmake -G Ninja \
    -DLLVM_ROOT_DIR=path/to/llvm/root \
    -DAFLPP_DIR=path/to/aflplusplus/root ../
```
