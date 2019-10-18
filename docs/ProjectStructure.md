# Project Structure

CAF consists of 3 sub-projects:

1. A LLVM pass to extract necessary information from LLVM IR. This project is maintained under the `llvm-plugin` directory;

2. A small, header-only library that defines the internal representation of types, constructors and APIs, and interfaces manipulating those structures. This project is maintained under the `caf` directory.

3. An alternative `afl-fuzz.c` program that can be used as a drop-in replacement of AFL's original definition which defines the behavior of the CAF's fuzzing server. This project is maintained under the `afl` directory.
