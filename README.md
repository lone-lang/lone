# lone
The standalone Linux Lisp

Lone is a freestanding Lisp interpreter
designed to run directly on top of the Linux kernel
with full support for Linux system calls.
It has zero dependencies, not even the C standard library.

```lisp
(import (lone) (linux system-call))

(system-call 'write 1 "Hello, world!" 13)
```

## Features

 - Data types
   - [x] Integers (signed, 56-bit)
   - [x] Texts (UTF-8)
   - [x] Byte buffers
   - [x] Symbols (interned)
   - [x] Lists (linked)
   - [x] Vectors (contiguous arrays)
   - [x] Tables (hash tables with prototypal inheritance)
   - [x] Nil, true, false
   - [ ] Floating point
 - Functions
   - [x] Lambdas and closures
   - [x] Variadic
   - [x] FEXPR (unevaluated arguments)
   - [x] Primitive (built-in C functions)
   - [x] Generators and yield
   - [x] Delimited continuations
 - Modules
   - [x] Module system with import/export
   - [x] Intrinsic modules (lone, math, list, text, bytes, table, vector, linux)
   - [x] File system module loading
   - [x] Embedded ELF segment modules
 - Linux integration
   - [x] System calls
   - [x] Process parameters (arguments, environment, auxiliary vector)
   - [x] Loadable embedded ELF segment (`PT_LONE`)
   - [x] Tools (`lone-embed`)
 - Runtime
   - [x] Step-based virtual machine
   - [x] Freestanding memory allocator
   - [x] Mark-sweep-compact garbage collector
   - [x] Tagged value representation with inline small values
   - [x] FNV-1a hashing

## Building

Lone is built with GNU Make.

    make                    # Build the lone interpreter
    make lone               # Build just the interpreter
    make tools              # Build standalone tools
    make tests              # Build C test programs
    make all                # Build everything
    make test               # Run the test suite
    make clean              # Clean all build artifacts
    make clean lone         # Clean and rebuild interpreter

Build configuration:

    make CFLAGS=-g                  # Set compiler options
    make CC=clang                   # Use alternative compiler (cc, gcc, clang)
    make LD=mold                    # Use alternate linker (ld, lld, mold)
    make LTO=yes                    # Enable Link Time Optimization
    make TARGET=aarch64 UAPI=/path  # Cross-compile for another architecture
                                    # Implies CC=clang

Currently supported architectures:

 - `x86_64`
 - `aarch64`

## Testing

Lone has an automated test suite with over 300 test cases.
Almost all tests are lone lisp programs.

    make test
    scripts/test.bash test/ build/$ARCH/
    scripts/test.bash test/ build/$ARCH/ $TEST_NAME

<details>
<summary>How tests are organized</summary>

Tests live under `test/` as directory trees.
Each leaf directory contains test files:

 - `input` — standard input (required)
 - `output` — expected standard output
 - `error` — expected standard error
 - `arguments` — CLI arguments, one per line
 - `environment` — environment variables, one per line
 - `status` — expected exit status (0 assumed if absent)

Test names are derived from the directory path, minus the `test/` prefix:

    name(test/lone/lisp/modules/intrinsic/math/+/arity)
    = lone/lisp/modules/intrinsic/math/+/arity

An `executable` file in the test hierarchy names
which program to run (resolved relative to `build/$ARCH/`).
It is inherited: a parent directory's `executable`
applies to all descendants unless overridden.

New tests can be created with:

    scripts/test.new $TEST_NAME [input output error status arguments environment]

</details>

## Project structure

<details>
<summary>Description of the structure of the repository</summary>

    lone/                   # The lone repository
    ├── architecture/       # Architecture-specific code
    ├── build/              # The build tree (generated)
    ├── documentation/      # Design documents
    ├── include/            # Header files
    ├── scripts/            # Development tools and test suite
    ├── source/             # Source files
    ├── test/               # The lone test suite
    ├── GNUmakefile         # The build system
    ├── LICENSE.AGPLv3      # Full license text of the GNU AGPLv3
    ├── README.md           # This README file
    ├── .gdbinit            # GDB visualization functions for lone's data structures
    └── .github/            # GitHub Actions CI

    lone/include/
    ├── lone.h                             # Top-level include
    └── lone/
        ├── hash/
        │   └── fnv_1a.h                   # Fowler–Noll–Vo hash function
        ├── lisp/
        │   ├── modules/
        │   │   ├── intrinsic/
        │   │   │   ├── bytes.h            # Byte buffer manipulation
        │   │   │   ├── linux.h            # Linux system calls and process parameters
        │   │   │   ├── list.h             # List manipulation functions
        │   │   │   ├── lone.h             # Core language primitives
        │   │   │   ├── math.h             # Mathematical functions
        │   │   │   ├── table.h            # Table manipulation functions
        │   │   │   ├── text.h             # Text manipulation functions
        │   │   │   └── vector.h           # Vector manipulation functions
        │   │   ├── embedded.h             # Embedded ELF segment modules
        │   │   └── intrinsic.h            # Bulk initializer for all built-in modules
        │   ├── definitions.h              # Lisp layer constants and macros
        │   ├── garbage_collector.h        # Mark-sweep-compact garbage collector
        │   ├── hash.h                     # Lisp value hashing
        │   ├── heap.h                     # Value heap management
        │   ├── machine.h                  # Step-based virtual machine
        │   ├── module.h                   # Module loading and management
        │   ├── printer.h                  # Writes lone values into text
        │   ├── reader.h                   # Reads text into lone values
        │   ├── segment.h                  # ELF segment handling
        │   ├── types.h                    # Lisp type definitions and tags
        │   └── utilities.h               # Lisp utility functions
        ├── memory/
        │   ├── allocator.h                # First-fit block allocator
        │   ├── array.h                    # Dynamic array operations
        │   └── functions.h                # Memory moving and filling
        ├── auxiliary_vector.h             # Linux auxiliary vector access
        ├── bits.h                         # Bit manipulation utilities
        ├── definitions.h                  # Core constants and macros
        ├── elf.h                          # ELF format definitions
        ├── hash.h                         # General hashing functions
        ├── linux.h                        # Linux system calls
        ├── lisp.h                         # Lisp interpreter initialization
        ├── memory.h                       # Memory subsystem initialization
        ├── segment.h                      # ELF segment operations
        ├── stack.h                        # Stack utilities
        ├── system.h                       # System state management
        ├── test.h                         # C test framework
        ├── types.h                        # Primitive and aggregate type definitions
        └── utilities.h                    # General utility functions

    lone/source/
    ├── lone/                              # Mirrors the include/ directory structure
    │   ├── hash/
    │   │   └── fnv_1a.c
    │   ├── lisp/
    │   │   ├── modules/
    │   │   │   ├── intrinsic/
    │   │   │   │   ├── bytes.c
    │   │   │   │   ├── linux.c
    │   │   │   │   ├── list.c
    │   │   │   │   ├── lone.c
    │   │   │   │   ├── math.c
    │   │   │   │   ├── table.c
    │   │   │   │   ├── text.c
    │   │   │   │   └── vector.c
    │   │   │   ├── embedded.c
    │   │   │   └── intrinsic.c
    │   │   ├── value/                     # Value type constructors and operations
    │   │   │   ├── bytes.c
    │   │   │   ├── continuation.c
    │   │   │   ├── function.c
    │   │   │   ├── generator.c
    │   │   │   ├── integer.c
    │   │   │   ├── list.c
    │   │   │   ├── module.c
    │   │   │   ├── primitive.c
    │   │   │   ├── symbol.c
    │   │   │   ├── table.c
    │   │   │   ├── text.c
    │   │   │   └── vector.c
    │   │   ├── garbage_collector.c
    │   │   ├── hash.c
    │   │   ├── heap.c
    │   │   ├── machine.c
    │   │   ├── module.c
    │   │   ├── printer.c
    │   │   ├── reader.c
    │   │   ├── segment.c
    │   │   ├── types.c
    │   │   └── utilities.c
    │   ├── memory/
    │   │   ├── allocator.c
    │   │   ├── array.c
    │   │   └── functions.c
    │   ├── auxiliary_vector.c
    │   ├── bits.c
    │   ├── elf.c
    │   ├── hash.c
    │   ├── linux.c
    │   ├── lisp.c
    │   ├── memory.c
    │   ├── segment.c
    │   ├── stack.c
    │   ├── system.c
    │   ├── test.c
    │   ├── types.c
    │   └── utilities.c
    ├── tests/                             # C test programs
    │   ├── lone/
    │   │   ├── bits.c                     # Bit manipulation tests
    │   │   ├── stack.c                    # Stack operation tests
    │   │   └── types.c                    # Type system tests
    │   └── system-call.c                  # Linux system call tests
    ├── tools/
    │   └── lone-embed.c                   # Embeds code into a lone interpreter executable
    └── lone.c                             # The main lone function

    lone/architecture/
    └── $ARCH/
        └── include/
            └── lone/architecture/
                ├── linux/
                │   ├── entry_point.c      # Process start code
                │   └── system_calls.c     # Linux system call stubs
                └── garbage_collector.c    # Register spilling code

    lone/build/
    └── $ARCH/
        ├── include/
        │   └── lone/
        │       └── NR.c                   # Generated system call table initializers
        ├── objects/                       # Compiled object files
        ├── prerequisites/                 # Dependency tracking files
        ├── tests/
        │   └── lone/                      # Compiled C test programs
        ├── tools/
        │   └── lone-embed                 # Compiled embedding tool
        ├── NR.list                        # System call list for the target
        └── lone                           # The built lone interpreter

    lone/scripts/
    ├── NR.filter                          # Extracts system call definitions
    ├── NR.generate                        # Generates system call table initializers
    ├── create-symlinked-directory.bash     # Build system helper
    ├── test.bash                          # The automated test suite runner
    └── test.new                           # New test case creation script

    lone/test/
    ├── lone/
    │   ├── bits/                          # Bit manipulation tests
    │   ├── lisp/
    │   │   ├── language/
    │   │   │   ├── semantics/             # Language semantics tests
    │   │   │   └── syntax/                # Reader and parser tests
    │   │   ├── modules/
    │   │   │   └── intrinsic/             # Intrinsic module tests
    │   │   │       ├── bytes/
    │   │   │       ├── linux/
    │   │   │       ├── list/
    │   │   │       ├── lone/
    │   │   │       ├── math/
    │   │   │       ├── table/
    │   │   │       ├── text/
    │   │   │       └── vector/
    │   │   └── programs/                  # Integration tests
    │   └── types/                         # Type system tests
    ├── c/
    │   └── linux/
    │       └── system-call/               # C-level system call tests
    ├── tools/
    │   └── lone-embed/                    # Embedding tool tests
    └── executable                         # Default test executable

    lone/.github/
    ├── workflows/
    │   └── lone.yml                       # Build, test and CodeQL analysis
    └── FUNDING.yml                        # Funding information

</details>
