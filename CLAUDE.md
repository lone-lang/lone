# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code)
when working with code in this repository.

## Build Commands

```bash
make                    # Build the lone interpreter
make lone               # Build just the interpreter
make tools              # Build standalone tools
make all                # Build everything
make test               # Run full test suite
make clean              # Clean all build artifacts
make clean lone         # Clean and rebuild interpreter
make clean all          # Clean and rebuild everything
```

Build configuration options:
```bash
make CFLAGS=-g                  # Add debug symbols
make LD=mold                    # Use alternate linker (ld, lld, mold)
make LTO=yes                    # Enable Link Time Optimization
make TARGET=aarch64 UAPI=/path  # Cross-compile
                                # (requires Linux UAPI headers for target)
```

## Tests

Tests live under `test/` as directory trees.
Each leaf directory contains:

  - `input` — stdin (required)
  - `output` — expected stdout (optional)
  - `error` — expected stderr (optional)
  - `arguments` — CLI args, one per line
  - `environment` — env vars, one per line
  - `status` — expected exit code (0 assumed)

To run all tests manually without using make:

```bash
scripts/test.bash test/ build/$ARCH/
```

To run a specific test manually:
```bash
scripts/test.bash test/ build/$ARCH/ $TEST
```

Where `$TEST` is the test name.
Test names are the directories leading up to
the test's input file, minus the `test/` prefix.

```
name(test/example/test/input) = example/test
```

To create a new test:
```bash
scripts/test.new $TEST_NAME [input output error status arguments environment]
```
This creates the directory and opens the files in `$EDITOR`.

### Test executable selection

Tests don't all run against the `lone` interpreter.
An `executable` file in the test hierarchy names
which program to run (resolved relative to `build/$ARCH/`).
It is inherited: a parent directory's `executable`
applies to all descendants unless overridden.

  - `test/executable` contains `lone` (default for most tests)
  - `test/lone/types/executable` contains `tests/lone/types`
  - `test/lone/bits/executable` contains `tests/lone/bits`
  - `test/c/linux/system-call/executable` contains `tests/system-call`

C test programs live in `source/tests/` and are built
alongside the interpreter by `make tests`.

## Architecture

Lone is a **freestanding Lisp interpreter**
that runs directly on Linux
with zero external dependencies:
no libc, no standard library.
Everything is statically linked.

### Two-level system design

Lone separates low-level system state from the Lisp interpreter:

  - `struct lone_system` — memory allocator and hash state.
    Initialized first from a static 1 MiB buffer.
    Uses a first-fit block allocator with splitting/coalescing.

  - `struct lone_lisp` — the interpreter itself.
    Contains the value heap, symbol table, module registry,
    top-level environment, and native stack pointer for GC.

This separation means `lone_system` functions
(allocation, hashing) never depend on the Lisp layer.

### Initialization sequence

```
architecture entry point  (architecture/$ARCH/include/lone/architecture/linux/entry_point.c)
  inline asm: parse argc/argv/envp/auxv from stack, call lone()
    → lone_system_initialize     allocator + hash state from static buffer + auxv random
    → lone_lisp_initialize       heap, symbol table, module table, import/export primitives
    → intrinsic module init      register all built-in modules
    → module path setup          ".", ~/.lone/modules, ~/.local/lib/lone/modules, /usr/lib/lone/modules
    → load embedded segment      run code from custom ELF segment if present
    → read and evaluate stdin    the null module
```

### Value representation

Values are tagged words (`struct lone_lisp_value { long tagged; }`).
The low byte is the tag. The upper 56 bits are data.

  - bit 0 = 0 — heap value (40-bit index + 16-bit metadata)
  - bit 0 = 1 — non-heap (56-bit data payload)
  - `0x01` — integer (56-bit signed)
  - `0x03` — nil
  - `0x05` — true
  - `0x07` — false
  - `0x81`..`0x8F` — inline symbol (up to 7 bytes in-word)
  - `0x91`..`0x9F` — inline text
  - `0xA1`..`0xAF` — inline bytes

Heap values live in a flat `mmap`'d array of `lone_lisp_heap_value`
structs (64-byte aligned). Three separate bitmaps (live, marked, pinned)
track per-value state. The heap grows via `mremap` with `MAYMOVE`.
Values encode array **indexes**, not pointers, making them
position-independent across remaps.

Heap-allocated types: Module, Function, Primitive,
Continuation, Generator, List, Vector, Table, Symbol, Text, Bytes.

See `documentation/tagged-values.md` for the full design.

### Garbage collector

Three-phase mark-sweep-compact GC:

  1. **Mark** — walks roots: lisp machine stack (precise, typed frames),
     native C stack + registers (conservative scanner),
     interpreter internal values (symbol table, modules, etc.)
  2. **Sweep** — deallocates resources of unmarked values,
     clears live bits
  3. **Compact** — moves live values down, writes forwarding indexes,
     rewrites all references across the entire interpreter

Architecture-specific code (`architecture/$ARCH/include/lone/architecture/garbage_collector.c`)
spills registers to the stack before the conservative scan.

### Machine (evaluator)

The evaluator is a **step-based virtual machine** (`source/lone/lisp/machine.c`)
rather than a recursive tree-walker. The machine struct holds registers:
expression, environment, module, value (result), applicable, list, unevaluated.

`lone_lisp_machine_cycle()` is a single-step function driven by `machine->step`.
Steps include expression evaluation, operator evaluation,
operand evaluation/accumulation, application, and sequence evaluation.
The machine stack stores typed frames (values, integers, return steps,
delimiters for function/continuation/generator boundaries).

Primitives (C functions) use `LONE_LISP_PRIMITIVE(name)` and interact
with the machine via a multi-step protocol: they receive a `step` counter,
push/pop values on the machine stack, set `machine->step` to request
sub-evaluations, and return the next step number (0 = done).

### Function flags (FEXPR support)

Functions and primitives carry two flags:
  - `evaluate_arguments` — whether operands are evaluated before application
  - `evaluate_result` — whether the result is re-evaluated after

This enables FEXPRs (receiving unevaluated syntax)
and macro-like code generation.

### Module system

  - Top-level environment contains only `import` and `export`
  - Each module gets a clean environment inheriting from top-level
  - `import`/`export` are FEXPR primitives (unevaluated arguments)
  - Modules loaded from disk are `.ln` files found on the module path
  - The null module (unnamed) reads from stdin
  - Embedded modules come from a custom ELF segment (`PT_LONE = 0x6c6f6e65`)

### Header/source convention

`include/lone/` mirrors `source/lone/` — each `.c` has a `.h`.
Architecture-specific files are under `architecture/$ARCH/include/`
and are `#include`'d directly into the corresponding source file
(e.g., `garbage_collector.c` includes `architecture/garbage_collector.c`).

## Naming conventions

All symbols are prefixed with `lone_`:
  - System layer: `lone_system_*`, `lone_memory_*`, `lone_allocate`/`lone_deallocate`
  - Lisp layer: `lone_lisp_*`
  - Types: `lone_lisp_is_*` (predicates), `lone_lisp_*_create` (constructors)
  - Machine: `lone_lisp_machine_*`
  - Modules: `lone_lisp_modules_intrinsic_*_initialize`, `lone_lisp_primitive_*`
  - Linux syscalls: `linux_*`
  - Enum values: `LONE_LISP_TAG_*`, `LONE_LISP_MACHINE_STEP_*`

## Compiler flags

The build enforces freestanding constraints:
`-static -ffreestanding -nostdlib -fno-omit-frame-pointer`.
The entry point is `lone_start` (`-Wl,-elone_start`).
`-fno-strict-aliasing` and `-fwrapv` are always applied
regardless of user CFLAGS.

## CI

GitHub Actions (`.github/workflows/lone.yml`)
tests against a matrix of compilers (cc, gcc, clang)
and linkers (ld, lld, mold) on x86_64, plus CodeQL analysis.
