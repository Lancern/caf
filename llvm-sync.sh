#!/usr/bin/env bash

if [[ ! -e $LLVM_DIR ]]; then
    # LLVM_DIR does not exist.
    echo "LLVM cannot be found at $LLVM_DIR".
    exit
elif [[ ! -d $LLVM_DIR ]]; then
    echo "ERROR: $LLVM_DIR is not a directory!"
    exit
fi

LLVM_TRANSFORMS_DIR=$LLVM_DIR/lib/Transforms
LLVM_TRANSFORMS_CMAKE_FILE=$LLVM_TRANSFORMS_DIR/CMakeLists.txt
CAF_PASS_DIR=$LLVM_TRANSFORMS_DIR/CAFDriver

if [[ ! -e $CAF_PASS_DIR ]]; then
    # Pass directory does not exist.
    echo "CAF pass directory does not exist. Creating it..."
    mkdir $CAF_PASS_DIR
elif [[ ! -d $CAF_PASS_DIR ]]; then
    echo "ERROR: $CAF_PASS_DIR is a file, not a directory."
    exit
fi

echo "The pass directory $CAF_PASS_DIR is located."
echo "Syncing files..."

cp $CAF_PASS_DIR/CAFMeta.hpp caf/
cp $CAF_PASS_DIR/CAFCodeGen.hpp llvm-plugin/
cp $CAF_PASS_DIR/CAFDriver.cpp llvm-plugin/
cp $CAF_PASS_DIR//CAFSymbolTable.hpp llvm-plugin/
cp $CAF_PASS_DIR/CMakeLists.txt llvm-plugin/

echo "Done."
