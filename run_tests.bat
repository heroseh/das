@ECHO OFF
ECHO running das_test...
IF EXIST "das_test.exe" (
	DEL das_test.exe
)

clang.exe -g -o das_test.exe das_test.c

IF EXIST "das_test.exe" (
	das_test.exe
)

ECHO -----------------
ECHO running cpp_das_test...
IF EXIST "cpp_das_test.exe" (
	DEL cpp_das_test.exe
)

clang++.exe -g -o cpp_das_test.exe cpp_das_test.cpp

IF EXIST "cpp_das_test.exe" (
	cpp_das_test.exe
)

