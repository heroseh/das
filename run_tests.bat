@ECHO OFF
ECHO running das_test with clang...
IF EXIST "das_test.exe" (
	DEL das_test.exe
)

clang.exe -g -o das_test.exe das_test.c

IF EXIST "das_test.exe" (
	das_test.exe
)

ECHO running das_test with Visual Studio cl...
IF EXIST "das_test.exe" (
	DEL das_test.exe
)

cl.exe das_test.c

IF EXIST "das_test.exe" (
	das_test.exe
)

