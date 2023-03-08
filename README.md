# lone
The standalone Linux Lisp

Lone is a freestanding Lisp interpreter
designed to run directly on top of the Linux kernel
with full support for Linux system calls.
It has zero dependencies, not even the C standard library.

Currently in the early stages of development.

## Building

Lone is built by a simple GNU Make file.
Any of the following commands can be used:

    make
    make lone
    make clean
    make clean lone

## Testing

Lone has an automated test suite that exercises language features.
Any of the following commands can be used to run it:

    make test
    make clean test
    ./test.bash

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

    lone/                         # The lone repository
    ├── arch/                     # Architecture-specific code, one file each
    │   ├── aarch64.c             # System calls and process start for aarch64
    │   └── x86_64.c              # System calls and process start for x86_64
    ├── test/                     # The lone test suite
    │   └── arbitrary/tree/       # Arbitrary tree, determines test name, leaves contain test files
    │       ├── arguments         # Arguments passed, one per line
    │       ├── environment       # Environment variables set, one per line
    │       ├── input             # Standard input
    │       ├── output            # Expected standard output
    │       ├── error             # Expected standard error
    │       └── status            # Expected exit status
    ├── GNUmakefile               # The GNU Make file
    ├── LICENSE.AGPLv3            # GNU Affero General Public License version 3, full license text
    ├── lone                      # The lone executable produced by make
    ├── lone.c                    # The lone C source code
    ├── README.md                 # This README file
    ├── test.bash                 # The test script
    ├── test.new                  # The new test case creation script
    ├── .gdbinit                  # GDB visualization functions for lone's data structures
    └── .github/                  # GitHub-specific data
        └── workflows/            # GitHub Actions workflows
            ├── codeql.yml        # Automated code quality checker
            └── lone.yml          # Automated building and testing
