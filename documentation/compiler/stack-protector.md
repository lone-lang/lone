# Stack protector

When `-fstack-protector` is enabled, the compiler emits references
to runtime symbols that normally live in libc or compiler-rt.
Lone is freestanding. There is no libc. Lone must provide
those symbols all by itself. Without support for this,
passing `-fno-stack-protector` is required
to avoid missing symbol errors at link time.

## Compiler interface

With `-fstack-protector` or variants, the compiler emits code that:

  1. Reads `__stack_chk_guard` in the function prologue
  2. Copies it to the stack next to the return address
  3. Reads `__stack_chk_guard` in the function epilogue
  4. Compares it to the copy stored on the stack
  5. Calls `__stack_chk_fail` if the copy was modified

The two symbol names are the binary interface contract.
Their C types and semantics are ours to choose
as long as the compiler's code generation
requirements remain satisfied.

## Implementation

```c
unsigned long __stack_chk_guard;

__attribute__((noreturn))
void __stack_chk_fail(void)
{
    linux_exit(-1);
}
```

The stack check guard is a global zero-initialized integer.
An initializer writes 8 bytes from the auxiliary vector's
`AT_RANDOM` blob to it before any protected function runs.

```c
void lone_compiler_stack_protector_initialize(struct lone_bytes random)
{
    __stack_chk_guard = lone_u64_read(random.pointer);
}
```

The Linux kernel supplies every process with 16 bytes of randomness
via `AT_RANDOM` in the auxiliary vector. These random bytes can be
easily extracted via `lone_auxiliary_vector_random`. They are also
used to seed the FNV-1a hash with a random initial offset basis.

## Initialization

The canary's initialization must happen before any compiler-generated
epilogue reads `__stack_chk_guard` for the first time. This happens
in a dedicated `lone_start` function invoked directly by the
architecture-specific entry assembly before `lone` runs.

```c
__attribute__((no_stack_protector))
long lone_start(int argc, char **argv, char **envp, struct lone_auxiliary_vector *auxv);
```

`lone` itself is an ordinary function.
Programs that provide their own `lone`
functions, such as `lone-embed`, inherit
canary initialization from `lone_start`
automatically without needing any boilerplate.

The `lone_start` function serves as the only function in the program
that is marked with `no_stack_protector`. Without this attribute,
the function will receive stack protection. If this happens,
it will read the stack canary, which is zero at first,
then it will overwrite the canary by initializing it.
The epilogue will then trigger stack protection
when it compares the initialized canary with zero:
`__stack_chk_fail` will be called, aborting the process
despite the fact nothing smashed anything.

## Compilation

The stack protector functions are compiled unconditionally.
The build system places each function and global variable
into its own ELF section via `-ffunction-sections` and
`-fdata-sections`, then garbage-collects unused sections
via `-Wl,--gc-sections`. Unreferenced symbols are dropped
from the final executable.

The call to the initializer from `lone_start` is
guarded by `#ifdef LONE_COMPILER_STACK_PROTECTOR`.
The macro is defined only when stack protection
is enabled. Without the definition `lone_start`
skips both the auxiliary-vector walk and the
initializer call entirely.

Nothing reachable references `__stack_chk_guard`, nothing
calls `__stack_chk_fail`, and nothing calls
`lone_compiler_stack_protector_initialize`. The linker
drops all three along with the initializer function
itself. Runtime cost is zero.

## Build system integration

A makefile flag controls whether stack protection is enabled:

```
make STACK_PROTECTOR=yes
```

With the flag set, `-fstack-protector-strong` replaces
`-fno-stack-protector`, and `-mstack-protector-guard=global`
is added to direct the compiler at the `__stack_chk_guard`
global.

The `guard=global` override is necessary on targets where
the compiler's default canary lives in thread-local storage.
GCC on x86_64 Linux defaults to reading the canary from a
thread-local storage slot, at `%fs:0x28`. Lone does not
set up thread-local storage, so this would dereference
address `0x28` and crash on every protected function.
The override points the compiler at the global variable
lone provides.
