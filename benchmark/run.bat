@ECHO off

ECHO start build of das_main at %time%
clang.exe -o das_main.exe das_main.c
ECHO build of das_main finished at %time%

ECHO.

ECHO start build of cpp_vector_trio_main at %time%
clang++.exe -o cpp_vector_trio_main.exe cpp_vector_trio_main.cpp
ECHO build of cpp_vector_trio_main finished at %time%

ECHO.

ECHO start build of cpp_vector_main at %time%
clang++.exe -o cpp_vector_main.exe cpp_vector_main.cpp
ECHO build of cpp_vector_main finished at %time%

