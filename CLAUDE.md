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
make STACK_PROTECTOR=yes        # Enable the stack protector
make TARGET=aarch64 UAPI=/path  # Cross-compile
                                # (requires Linux UAPI headers for target)
```

## Tests

540 tests live under `test/` as directory trees.
Each leaf directory contains:

  - `input` — stdin (required)
  - `output` — expected stdout (optional)
  - `error` — expected stderr (optional)
  - `arguments` — CLI args, one per line
  - `environment` — env vars, one per line
  - `status` — expected exit code (0 assumed)
  - `program-name` — argv[0] override (optional)
  - `script` — if present and executable, runs instead
    of the test executable

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

  - `test/executable` → `lone` (default for most tests)
  - `test/lone/types/executable` → `tests/lone/types`
  - `test/lone/bits/executable` → `tests/lone/bits`
  - `test/lone/reader/executable` → `tests/reader`
  - `test/c/linux/system-call/executable` → `tests/system-call`

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
    The slab allocator has 10 size classes (8, 16, 32, ...4096 bytes),
    each backed by 64 KiB slabs allocated via `mmap`.
    Allocations larger than 4096 bytes go directly to `mmap`.
    Each slab maintains a free list for O(1) reuse.
    Hashing uses FNV-1a with a randomized offset basis
    derived from the auxiliary vector's `AT_RANDOM` bytes.

  - `struct lone_lisp` — the interpreter itself.
    Contains the value heap, symbol table, module registry,
    top-level environment, and native stack pointer for GC.

This separation means `lone_system` functions
(allocation, hashing) never depend on the Lisp layer.

### Initialization sequence

The ELF entry point is the `lone_entry` symbol
(`-Wl,-elone_entry`). The architecture-specific entry
is inline assembly that parses argc/argv/envp/auxv
from the stack and then calls `lone_start`, which in
turn calls `lone`.

```
architecture entry point  (architecture/$ARCH/include/lone/architecture/linux/entry.c)
  inline asm: parse argc/argv/envp/auxv from stack, call lone_start
    → lone_compiler_stack_protector_initialize   canary from AT_RANDOM (if STACK_PROTECTOR)
    → lone()
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

Non-heap tags:
  - `0x01` — integer (56-bit signed)
  - `0x03` — nil
  - `0x05` — true
  - `0x07` — false
  - `0x81`..`0x8F` — inline symbol (0..7 bytes in-word)
  - `0x91`..`0x9F` — inline text
  - `0xA1`..`0xAF` — inline bytes

Heap tags (even, bit 0 = 0): module, function, primitive, continuation,
generator, list, vector, table, symbol, text, bytes.

The 16-bit metadata field in a heap-value tagged word carries
per-value flags that must survive garbage collection:
FEXPR flags (`evaluate_arguments`, `evaluate_result`),
function arity (4 bits with a 15-value overflow sentinel),
and symbol hash bits (8 bits, for hash-table lookup acceleration).

Heap values live in a flat `mmap`'d array of `lone_lisp_heap_value`
structs (64-byte aligned). Initial capacity is 1M values.
Three separate bitmaps (live, marked, pinned) track per-value state.
The heap doubles in size via `mremap` with `MAYMOVE`.
Values encode 40-bit array **indexes**, not pointers,
making them position-independent across remaps.

Heap-allocated types: Module, Function, Primitive,
Continuation, Generator, List, Vector, Table, Symbol, Text, Bytes.

Bytes values carry a `frozen` flag. Frozen bytes are immutable
and may be hashed or used as table keys; unfrozen bytes cannot.
Inline bytes are always frozen. Bytes literals (`b"..."`) are
frozen at read time.

See `documentation/tagged-values.md` for the full design.

### Garbage collector

Three-phase mark-sweep-compact GC:

  1. **Mark** — walks roots: interpreter internals (symbol table,
     modules, top-level environment), lisp machine stack
     (precise, typed frames), native C stack + registers
     (conservative scanner). Conservative pointers pin
     their targets so they are not moved during compaction.
  2. **Sweep** — deallocates owned resources of unmarked values
     (bytes buffers, vector arrays, table storage, continuation
     frames, generator stacks), clears live bits, updates
     heap bounds.
  3. **Compact** — scans for gaps left by sweep, moves unpinned
     live values down, writes forwarding indexes in vacated slots,
     then rewrites all references across the entire interpreter
     (roots, stack, interior pointers of every live value).

Architecture-specific code (`architecture/$ARCH/include/lone/architecture/garbage_collector.c`)
spills all general-purpose registers to the stack before the
conservative scan.

### Machine (evaluator)

The evaluator is a **step-based virtual machine** (`source/lone/lisp/machine.c`)
rather than a recursive tree-walker. The machine struct holds registers:
expression, environment, module, value (result), applicable, list, unevaluated.

`lone_lisp_machine_cycle()` is a single-step function driven by `machine->step`.
Steps include expression evaluation, operator evaluation,
operand evaluation/accumulation, application, and sequence evaluation.

The machine stack is a separately `mmap`'d region that grows via `mremap`.
Initial size is 256 frames, maximum 65536, doubling on overflow
and shrinking when capacity exceeds 8x the initial size.
Frames are 8-byte tagged words sharing the value tagging scheme.
Frame tags include function, continuation, interceptor, and generator
delimiters, plus step and primitive-step markers.
Generators get independent stacks (initial size 128 frames)
that are swapped with the caller's stack on yield/resume.

Primitives (C functions) use `LONE_LISP_PRIMITIVE(name)` and interact
with the machine via a multi-step protocol: they receive a `step` counter,
push/pop values on the machine stack, set `machine->step` to request
sub-evaluations, and return the next step number (0 = done).

### Machine steps

The machine cycle advances through these steps:

  - `EVALUATE` — evaluate expression: self-evaluating types return
    immediately, symbols are looked up, lists decompose into
    operator + operands
  - `EVALUATED_OPERATOR` — check operator is applicable,
    decide whether to evaluate operands
  - `OPERAND_EVALUATION` — evaluate one operand
  - `OPERAND_ACCUMULATION` — collect evaluated operand, loop
  - `LAST_OPERAND_ACCUMULATION` — collect final operand, build list
  - `APPLICATION` — dispatch by type: function, primitive,
    continuation, generator, vector, or table
  - `AFTER_APPLICATION` — restore caller steps
  - `SEQUENCE_EVALUATION` — evaluate first expression in body
  - `SEQUENCE_EVALUATION_NEXT` — continue body; last expression
    gets tail call optimization
  - `RESUME_PRIMITIVE` — re-enter a suspended primitive
  - `TAIL_RETURN` — complete a tail call
  - `GENERATOR_RETURN` — swap stacks back after generator yields
  - `HALT` — stop the machine

### Multi-step primitive protocol

Primitives return a `long` step number:

  - `> 0` — suspend: the machine evaluates whatever the primitive
    put in `machine->expression` (or applies `machine->applicable`),
    then resumes the primitive at the returned step
  - `0` — done: result is on the machine stack
  - `-1` — tail evaluate: evaluate `machine->expression` in the
    caller's tail position
  - `-2` — tail apply: apply `machine->applicable` to `machine->list`
    in the caller's tail position

This lets complex primitives (`intercept`, `signal`, `if`, `begin`)
request sub-evaluations without recursive C calls.

### Tail call optimization

The last expression in a function body is evaluated in tail
position: the machine pops the function delimiter and replaces
the return step with `TAIL_RETURN`. This reuses the same stack
depth for arbitrary chains of tail calls. Primitives opt in
by returning `-1` (tail evaluate) or `-2` (tail apply).

### Signal and intercept

Lone uses a signal/intercept mechanism for error handling
and non-local control flow, not exceptions.

`(intercept clauses body ...)` establishes an interceptor
on the machine stack. `(signal tag value)` walks the stack
for a matching interceptor. Clauses match by:

  - `()` — wildcard, matches any signal
  - symbol — exact tag equality
  - applicable — called as predicate on the tag

Handler arity determines behavior:

  - arity 1: `(handler value)` — handle and return
  - arity 2: `(handler value continuation)` — handle with
    captured continuation for resumption

A dispatching flag on each interceptor delimiter prevents
re-entrant matching: signals raised inside a matcher
propagate to the next interceptor up the stack.

When a signal reaches a generator boundary with no matching
interceptor, the generator terminates and the signal
propagates to the caller's stack.

### Continuations and generators

`(control body handler)` establishes a continuation delimiter.
`(transfer value)` captures frames from the delimiter to the
current stack top as a first-class continuation, then tail-applies
the handler to `(value continuation)`. Applying a continuation
re-pushes the captured frames.

`(generator function)` creates a generator with its own
independent stack. Calling the generator swaps stacks and
evaluates `(function . arguments)`. `(yield value)` swaps
back to the caller's stack, suspending the generator.
Subsequent calls resume from the yield point.

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

The `lone-embed` tool under `source/tools/` injects a PT_LONE
segment into an existing lone binary, letting an interpreter ship
with precompiled lisp code that runs at startup.

Modules do not reload at runtime.

### Intrinsic modules

Eight built-in modules, registered at startup:

  - `lone` — core language: `begin`, `when`, `unless`, `if`,
    `let`, `set`, `quote`, `quasiquote`, `lambda`, `lambda!`,
    `intercept`, `signal`, `control`, `transfer`, `generator`,
    `yield`, `return`, `apply`, `print`, type predicates
    (`nil?`, `list?`, `symbol?`, `integer?`, `text?`, `true?`,
    `false?`, `vector?`, `table?`), comparison predicates
    (`identical?`, `equivalent?`, `equal?`), `freeze`, `frozen?`
  - `math` — `+`, `-`, `*`, `/`, `<`, `<=`, `>`, `>=`,
    `sign`, `zero?`, `positive?`, `negative?`
  - `list` — `construct`, `first`, `rest`, `map`, `reduce`, `flatten`
  - `vector` — `get`, `set`, `slice`, `each`, `count`
  - `table` — `get`, `set`, `delete`, `each`, `count`
  - `text` — `to-symbol`, `join`, `concatenate`
  - `bytes` — `new`, `zero?`, plus read/write primitives for
    u8/s8/u16/s16/u32/s32 in native, little-endian,
    and big-endian byte orders
  - `linux` — `system-call` primitive, plus values:
    `argument-count`, `arguments`, `environment`,
    `auxiliary-vector`, `system-call-table`

The `linux` module's syscall table is generated at build time.
`scripts/NR.filter` and `scripts/NR.generate` extract `__NR_*`
constants from the kernel UAPI headers and produce a C include
file at `build/$CONFIGURATION/include/lone/lisp/modules/intrinsic/linux/NR.c`.

### Reader

The reader (`source/lone/lisp/reader.c`) is a hand-written
recursive descent parser. It reads from a file descriptor
or a byte buffer and produces Lisp values.

Supported syntax:

  - integers (decimal, `+`/`-` prefix)
  - symbols (interned, unabbreviated English)
  - text (`"..."` with `\n`, `\t`, `\\`, `\"` escapes)
  - bytes (`b"..."`, frozen at read time)
  - lists (`(...)`)
  - vectors (`[...]`)
  - tables (`{...}`)
  - quote shorthand: `'x` → `(quote x)`
  - quasiquote: `` `x `` → `(quasiquote x)`,
    `,x` → `(unquote x)`, `,@x` → `(unquote* x)`

### Header/source convention

`include/lone/` mirrors `source/lone/` — each `.c` has a `.h`.
Architecture-specific files are under `architecture/$ARCH/include/`
and are `#include`'d directly into the corresponding source file
(e.g., `garbage_collector.c` includes `architecture/garbage_collector.c`).

### Supported architectures

  - `x86_64` — native builds
  - `aarch64` — native or cross-compiled (`make TARGET=aarch64 UAPI=/path`)

Each architecture provides three files under
`architecture/$ARCH/include/lone/architecture/`:

  - `linux/entry.c` — naked inline assembly entry point
  - `linux/system_calls.c` — syscall convention macros
  - `garbage_collector.c` — register spill for conservative GC scan

## Source tree

```
source/
  lone.c                        main: initialize system, lisp, run
  lone/
    start.c                     lone_start, called from asm entry
    system.c                    lone_system_initialize
    linux.c                     raw syscall wrappers
    elf.c                       ELF parsing
    auxiliary_vector.c          Linux auxv parsing
    types.c                     primitive type read/write (endianness)
    stack.c                     generic stack operations
    segment.c                   memory segment management
    bits.c                      bitwise utilities
    utilities.c                 general utilities
    test.c                      C test framework
    compiler/
      stack_protector.c         canary global + initialization
    hash/
      fnv_1a.c                  FNV-1a hash
    memory/
      allocator.c               slab allocator (allocate, reallocate, deallocate)
      functions.c               memcpy, memset, memmove, memcmp
      array.c                   dynamic array
    lisp/
      lisp.c                    lone_lisp_initialize
      types.c                   tagged value type operations
      heap.c                    heap allocation, growth, bitmaps
      reader.c                  S-expression parser
      printer.c                 value serialization
      hash.c                    lisp value hashing
      garbage_collector.c       mark, sweep, compact
      module.c                  module loading, import/export
      machine.c                 step-based evaluator
      machine/
        stack.c                 machine stack frames and delimiters
      value/
        symbol.c  integer.c  text.c  bytes.c
        list.c  vector.c  table.c
        function.c  primitive.c  continuation.c
        generator.c  module.c
      modules/
        intrinsic.c             registers all built-in modules
        embedded.c              loads PT_LONE ELF segment
        intrinsic/
          lone.c                core language primitives
          math.c                arithmetic and comparisons
          list.c                list operations
          vector.c              vector operations
          table.c               table operations
          text.c                text operations
          bytes.c               binary data operations
          linux.c               system call interface

source/tools/
  lone-embed.c                  injects PT_LONE segment into ELF binaries

source/tests/
  system-call.c                 minimal syscall test
  reader.c                      reader/parser test
  lone/
    types.c                     type read/write test
    bits.c                      bitwise operation test
    stack.c                     stack operation test
```

## Build output

Build artifacts go to `build/$CONFIGURATION/`
where `$CONFIGURATION` defaults to `$ARCH` (e.g., `x86_64`).
CI overrides it to encode the full matrix
(e.g., `x86_64+gcc+mold+lto+stack-protector`).

```
build/$CONFIGURATION/
  lone                          interpreter executable
  objects/lone/...              object files mirroring source/
  prerequisites/lone/...        gcc -MMD dependency files
  include/lone/.../NR.c         generated syscall table
  tools/
    lone-embed                  ELF embedding tool
  tests/
    system-call                 C test executables
    reader
    lone/types
    lone/bits
    lone/stack
```

## Documentation

In-tree documentation lives under `documentation/`:

  - `tagged-values.md` — full tagged value encoding design
  - `machine.md` — explicit-control evaluator, steps, stack frames
  - `table-shapes.md` — hash table implementation and shapes
  - `compiler/` — compiler support features (stack protector, etc.)
  - `heap/` — heap design (static initialization)
  - `intercept+signal/` — dispatch, primitives, reentrancy
  - `linux/` — memory map documentation
  - `modules/` — module reload policy

## Naming conventions

All symbols are prefixed with `lone_`:
  - System layer: `lone_system_*`, `lone_memory_*`, `lone_allocate`/`lone_deallocate`
  - Lisp layer: `lone_lisp_*`
  - Types: `lone_lisp_is_*` (predicates), `lone_lisp_*_create` (constructors)
  - Machine: `lone_lisp_machine_*`
  - Modules: `lone_lisp_modules_intrinsic_*_initialize`, `lone_lisp_primitive_*`
  - Linux syscalls: `linux_*`
  - Enum values: `LONE_LISP_TAG_*`, `LONE_LISP_MACHINE_STEP_*`

Lisp-level names are unabbreviated English
and predicate names end in `?` (`equal?`, `is-frozen?`).

## Compiler flags

The build enforces freestanding constraints:
`-static -ffreestanding -nostdlib -fno-omit-frame-pointer -fshort-enums`.
Undefined behavior traps via `-fsanitize-trap=all`.
Dead code is eliminated via `-ffunction-sections -fdata-sections`
plus `-Wl,--gc-sections`.
The ELF entry point is `lone_entry` (`-Wl,-elone_entry`).
`-fno-strict-aliasing` and `-fwrapv` are always applied
regardless of user CFLAGS.

When `STACK_PROTECTOR=yes`, the build adds
`-fstack-protector-strong -mstack-protector-guard=global
-DLONE_COMPILER_STACK_PROTECTOR`. The canary is a global
defined in `source/lone/compiler/stack_protector.c` and
initialized at startup from AT_RANDOM. The `lone_start`
function is exempt (via `__attribute__((no_stack_protector))`)
because it runs before the canary is in place.

## CI

GitHub Actions (`.github/workflows/lone.yml`) tests a matrix of:

  - platforms: x86_64 (archlinux), aarch64 (archlinuxarm)
  - compilers: cc, gcc, clang
  - linkers: ld, lld, mold, mold+spare-program-headers
  - LTO: on, off
  - stack protector: on, off

GCC+LLD+LTO and CC+LLD+LTO are excluded (GCC LTO IR
is incompatible with LLD). The full matrix is approximately
88 configurations across both platforms.
A separate CodeQL analysis job runs on x86_64.
