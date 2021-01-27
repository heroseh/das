#!/bin/sh

echo "time it takes to build das_main:"
time clang -o das_main das_main.c
echo "-------------------"

echo "time it takes to build cpp_vector_trio_main:"
time clang++ -o cpp_vector_trio_main cpp_vector_trio_main.cpp
echo "-------------------"

echo "time it takes to build cpp_vector_main:"
time clang++ -o cpp_vector_main cpp_vector_main.cpp
echo "-------------------"




