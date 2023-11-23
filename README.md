# lone
The standalone Linux Lisp

Lone is a freestanding Lisp interpreter
designed to run directly on top of the Linux kernel
with full support for Linux system calls.
It has zero dependencies, not even the C standard library.

Currently in the early stages of development.

    (import (lone) (linux system-call))

    (system-call 'write 1 "Hello, world!" 13)

## Building

Lone is built by a simple GNU Make file.

The following phony targets can be used:

    make
    make clean
    make lone
    make tools
    make clean lone
    make clean lone tools

The following variables are recognized:

    make CFLAGS=-g
    make LD=mold
    make LTO=yes
    make UAPI=/alternative/linux/uapi/headers
    make TARGET=x86_64 UAPI=/linux/uapi/headers/x86_64

Currently supported targets:

 - `x86_64`
 - `aarch64`

## Testing

Lone has an automated test suite that exercises language features.
Any of the following commands can be used to run it:

    make test
    make clean test
    scripts/test.bash [lone-executable [test-suite-directory]]

New tests are added by creating directories inside `test/`,
forming an arbitrary directory tree which determines the test name.
Leaf directories contain the test files used to exercise lone.

The following files may be present:

 - `arguments` Arguments to be passed, one per line.
 - `environment` Environment variables to be set, one per line.
 - `input` Standard input.
 - `output` Expected standard output.
 - `error` Expected standard error.
 - `status` Expected exit status code.

Tests are run in parallel by executing the lone interpreter
with the specified arguments, environment and standard input.
The status code and the data from standard output and error
are collected and compared against the expected parameters.
A successful test is one where actual outputs match expected outputs.

Only the `input` file is absolutely required.
The actual output and error data are only compared
if their respective `output` and `error` files are present.
If the `status` file is omitted,
the successful status code `0` is expected.

## Project structure

    lone/                 # The lone repository
    ├── build/            # The build tree
    ├── include/          # Header files
    ├── source/           # Source files
    ├── architecture/     # Architecture-specific tree
    ├── scripts/          # Development tools and test suite
    ├── test/             # The lone test suite
    ├── GNUmakefile       # The build system
    ├── LICENSE.AGPLv3    # Full license text of the GNU AGPLv3
    ├── README.md         # This README file
    ├── .gdbinit          # GDB visualization functions for lone's data structures
    └── .github/          # GitHub-specific data

    lone/include/                      # Added to compiler include directories
    └── lone/                          # Lone namespace
        ├── hash/                      # Hash function implementations
        │   └── fnv_1a.h               # Fowler–Noll–Vo hash function
        ├── lisp/                      # Lone lisp language features
        │   ├── constants.h            # Constants like nil and true
        │   ├── evaluator.h            # Evaluates lone values
        │   ├── printer.h              # Writes lone values into text
        │   └── reader.h               # Reads text into lone values
        ├── memory/                    # Lone's memory subsystem
        │   ├── allocator.h            # General memory block allocator
        │   ├── functions.h            # Memory moving and filling functions
        │   ├── garbage_collector.h    # The lone garbage collector
        │   └── heap.h                 # The lone value heap
        ├── modules/                   # Intrinsic lone modules
        │   ├── intrinsic.h            # Bulk initializer for all built-in modules
        │   ├── linux.h                # Linux system calls and process parameters
        │   ├── list.h                 # List functions
        │   ├── lone.h                 # Lone language primitives
        │   ├── math.h                 # Mathematical functions
        │   └── text.h                 # Text manipulation functions
        ├── struct/                    # Lone structure definitions
        │   ├── heap.h                 # Heap from where values are allocated
        │   └── memory.h               # Memory blocks managed by lone
        ├── value/                     # Functions for each type of value
        │   ├── bytes.h                # Creation and transfer functions
        │   ├── function.h             # Function and closure instantiation
        │   ├── integer.h              # Integer value creation and parsing
        │   ├── list.h                 # List construction and processing
        │   ├── module.h               # Module value creation
        │   ├── pointer.h              # Typed pointer value creation
        │   ├── primitive.h            # Primitive C function binding creation
        │   ├── symbol.h               # Symbol creation and interning
        │   ├── table.h                # Hash table creation and operations
        │   ├── text.h                 # Text value creation and C string transfers
        │   └── vector.h               # Vector creation and operations
        ├── definitions.h              # Defined constants and macros
        ├── hash.h                     # General hashing functions
        ├── linux.h                    # Linux system calls used by lone
        ├── lisp.h                     # Lone lisp interpreter initialization
        ├── memory.h                   # Lone memory subsystem initialization
        ├── modules.h                  # Module loading, search, path management
        ├── structures.h               # Includes all lone structure definitions
        ├── types.h                    # Basic type definitions and forward declarations
        ├── utilities.h                # Useful functions
        └── value.h                    # Blank slate lone value creation

    lone/source/            # Lone lisp implementation source code
    ├── tools/              # General use utilities and development tools
    │   └── lone-embed.c    # Embeds code into a lone interpreter executable
    ├── lone/               # Matches the structure or the include/ directory
    └── lone.c              # The main lone function

    lone/architecture/
    └── $ARCH/
        └── include/                       # Added to compiler include directories
            └── lone/architecture/
                ├── linux/
                │   ├── entry_point.c      # Process start code
                │   └── system_calls.c     # Linux system call stubs
                └── garbage_collector.c    # Register spilling code

    lone/build/
    └── $ARCH/                # The targeted architecture
        ├── include/          # Added to compiler include directories
        │   └── lone/
        │       └── NR.c      # Generated Linux system call table initializers
        ├── objects/          # Compiled object files; mirrors source tree structure
        ├── tools/            # Compiled utilities and tools
        │   └── lone-embed    # Embeds code into a lone interpreter executable
        ├── prerequisites/    # Prerequisite files; mirrors source tree structure
        ├── NR.list           # List of system calls found on the targeted Linux UAPI
        └── lone              # The built lone lisp freestanding executable

    lone/scripts/
    ├── NR.filter      # Extracts system call definitions from compiler output
    ├── NR.generate    # Generates C structure initializers for system call names and numbers
    ├── test.bash      # The automated test suite script
    └── test.new       # The new test case creation script

    lone/test/
    └── arbitrary/tree/    # Arbitrary tree, determines test name, leaves contain test files
        ├── arguments      # Arguments passed, one per line
        ├── environment    # Environment variables set, one per line
        ├── input          # Standard input
        ├── output         # Expected standard output
        ├── error          # Expected standard error
        └── status         # Expected exit status

    lone/.github/
    ├── workflows/        # GitHub Actions workflows
    │   ├── codeql.yml    # Automated code quality checker
    │   └── lone.yml      # Automated building and testing
    └── FUNDING.yml       # Funding information
