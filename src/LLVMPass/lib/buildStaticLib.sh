#!/bin/bash

clang ./caflib.cpp -c
ar crv ./caflib.a caflib.o

