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
echo "Copying files..."

cp lib/json/json.hpp $CAF_PASS_DIR/
cp caf/CAFMeta.hpp $CAF_PASS_DIR/
cp llvm-plugin/CAFCodeGen.hpp $CAF_PASS_DIR/
cp llvm-plugin/CAFDriver.cpp $CAF_PASS_DIR/
cp llvm-plugin/CAFSymbolTable.hpp $CAF_PASS_DIR/
cp llvm-plugin/CMakeLists.txt $CAF_PASS_DIR/

echo "Copying files complete."
echo "Checking LLVM transform cmake file $LLVM_TRANSFORMS_CMAKE_FILE..."

if [[ ! -e $LLVM_TRANSFORMS_CMAKE_FILE ]]; then
    echo "ERROR: could not find file $LLVM_TRANSFORMS_CMAKE_FILE."
    exit
fi

FOUND_ADD_CAF_DIR="NO"
< $LLVM_TRANSFORMS_CMAKE_FILE | while read CMAKE_LINE; do
    if [[ "add_subdirectory(CAFDriver)" == $CMAKE_LINE ]]; then
        FOUND_ADD_CAF_DIR="YES"
        break
    fi
done

if [[ "NO" == $FOUND_ADD_CAF_DIR ]]; then
    echo "Adding CAF subdirectory to LLVM transforms build..."
    echo "add_subdirectory(CAFDriver)" >> $LLVM_TRANSFORMS_CMAKE_FILE
fi

echo "Done."
