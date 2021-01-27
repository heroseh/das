# DAS - dynamically allocated storage

A feature rich dynamic allocation library.

# Features
- array-like stack (DasStk)
- double ended queue (DasDeque)
- custom allocator interface (DasAlctor)
- allocation API that use custom allocators (das_alloc, das_realloc, das_dealloc)
- virtual memory abstraction
- compiles as ISO C99
- simple API to keep user code as simple as possible.
	- zeroed memory is initialization
	- no type decorations required on the Stack and Deque API
	- few arguments as possible to promote an orthogonal API design
- no executable bloat
	- we define a single function for each operation, that can be used for all types of DasStk/DasDeque
- fast compile times
	- we do not generate any code, other than single line macros

# Supported Platforms
The code should ideally compile on any C99 compiler. C++ can be supported again but
it will only work through extensions as lots of C99 features do not work in ISO C++.
Patches are welcome to improve support for other compilers and language targets :)

Tested Platforms:
- Windows (clang)
- Linux (clang & gcc & tcc)
- MacOS (clang) - untested but should work since it's clang. let me know if you can test it!
- IOS (clang) - untested but should work since it's clang. let me know if you can test it!
- Android (gcc) - untested but should work since it's gcc. let me know if you can test it!

# How to Build

You only need to copy **das.h** and **das.c** into your project.

In one of your C compilation units, include the **das.h** header and **das.c** source files like so:

```
#include "das.h"
#include "das.c"
```

To use the library in other files, you need to include the **das.h** header.

```
#include "das.h"
```

# Documentation

Most structs, functions and macros have their documentation above them in the **das.h** header file.

If you are looking for examples, look in the **das_examples.c** file. There are comments to guide you through and it is suggested that you follow linearally to learn the API.<br>

In that file we have:
- stk_example()
	- initializing a stack
	- pushing and popping elements
	- inserting and removing elements
- deque_example()
	- initializing a double ended queue
	- pushing and popping elements from both ends
- alloc_example()
	- allocation API
- custom_allocator_example()
	- using a custom allocator (linear allocator)

# Project Structure

- das.c - main source file
- das.h - main header file
- das_examples.c - examples with comments, it is suggested that you follow linearally to learn the API.
- das_test.c - test file for C
- run_test.sh - posix shell script for compiling and running the two test files
	- compiles using gcc and clang
- run_test.bat - batch script for compiling and running the two test files
	- compiles using clang
- benchmark - a directory that holds a benchmark for compiling DasStk vs std::vector. See below for more info.

# Benchmarks
This benchmark tests **compiling** different DasStk implementations vs std::vector. In all tests except cpp_vector_trio_main, we are handling 130 different structures. For each of these, in the main function, a DasStk/std::vector is created and then elements get pushed on to it. These results are on an Intel i5 8250u CPU using Clang 10 on Linux in July 2020. No optimization level has been specified. The results are ordered by time where the fastest test comes first.

- das_main.c
	- using DasStk
	- compile-time: 0.174s
	- size: 51K
	- main_function_size: 28K
- cpp_vector_trio_main.cpp
	- using std::vector with 3 different structures instead of 130.
	- so the size will be less because of the small main function.
	- compile-time: 0.289s
	- size: 40K
	- main_function_size: 730B
- cpp_vector_main.cpp
	- using std::vector
	- compile-time: 3.430s
	- size: 1.1M
	- main_function_size: 30K

We do not have any run-time benchmarks, since these are unlikely to reflect what happens in the real world. At the end of the day, different implementations of stacks and deques are very similar.

However std::vector will generate a optimized function for every implementation. But realistically, where DAS passes in size and align arguments, std::vector have constants precompiled in. As far as I know, thats a very negligible win if any at all. This is the only benefit that I could think of. Personally I don't think it's worth the time to compile these different versions of std::vector just for these things.

In DAS, for each operation we have a single function that is used for every type. So there is a single push function for DasStk that is used for all types. These functions are not complicated and will fit in the instruction cache better. So if you are handling many different types of stacks or deques, then DAS is probably better. But again as far as I know, this is probably very negligible if you are not doing huge amounts of data.

I hope this helps, do your own tests if you need :)


