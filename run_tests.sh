#!/bin/sh

if test -f "das_test"; then
	rm das_test
fi

echo "running das_test on gcc..."
gcc -Wpedantic -Werror -g -o das_test das_test.c

if test -f "das_test"; then
	./das_test
fi

if test -f "das_test"; then
	rm das_test
	echo "running das_test on clang..."
	clang -Wpedantic -Werror -g -o das_test das_test.c
fi

if test -f "das_test"; then
	./das_test
fi

if test -f "das_test"; then
	rm das_test
	echo "running das_test on tcc..."
	tcc -Wpedantic -Werror -g -o das_test das_test.c
fi

if test -f "das_test"; then
	./das_test
fi

