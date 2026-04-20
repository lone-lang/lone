# Emitting signals from primitives

The `(signal tag value)` lisp primitive emits a signal
that `(intercept clauses body...)` can intercept.
However, it only works at the lisp level.
Functions to signal errors from C code
are also necessary. This document describes
how C primitives emit signals.

## The optional type

Functions that can fail return a small structure:

    struct lone_lisp_optional_value {
        bool present;
        struct lone_lisp_value value;
    };

The `present` boolean tells the caller
whether or not a value was returned.
If `true`, the `lone_lisp_value` is
valid and can be safely consumed.
If `false`, the value is undefined.
Functions may initialize it
but are not required to.

These values are returned in registers on both
`x86_64` and `aarch64`: `rax`/`rdx` and `x0`/`x1`,
respectively. It costs about the same as a
`struct lone_lisp_value` return. This avoids
out parameters and address taking, simplifying
the interface and leading to better optimization.

## The programming interface

Lone primitives emit signals using two functions:

    long
    lone_lisp_signal_cast(struct lone_lisp *lone,
                          struct lone_lisp_machine *machine,
                          struct lone_lisp_value tag,
                          struct lone_lisp_value value);

    long
    lone_lisp_signal_hail(struct lone_lisp *lone,
                          struct lone_lisp_machine *machine,
                          long step,
                          struct lone_lisp_value tag,
                          struct lone_lisp_value value);

Both set `machine->applicable` to the `signal` primitive
and `machine->list` to the `(tag value)` argument list.
They differ in how control flows after signaling.

### Cast

`cast` returns `-2`, causing the machine to tail apply
the `signal` primitive. The calling primitive's step
frame is popped: it abandons control permanently.
The signal either gets handled or the program terminates.

    if (!lone_lisp_is_integer(lone, index)) {
        return lone_lisp_signal_cast(lone, machine, type_error, index);
    }

When wrapping fallible functions:

    optional = succeed_or_fail(argument);

    if (!optional.present) {
        return lone_lisp_signal_cast(lone, machine, error, context);
    }

    actual = optional.value;

    /* Consume value... */

### Hail

`hail` sets `machine->step` to `APPLY` and returns `step`.
The calling primitive's step frame stays on the stack.
If an interceptor's handler captures a continuation
and later calls it, the primitive resumes at `step`
with `machine->value` set to the value passed to the
continuation. This enables primitives to recover
from errors and continue with a substitute value.

    case 0:
        optional = succeed_or_fail(argument);

        if (!optional.present) {
            lone_lisp_machine_push_value(lone, machine, partial);
            return lone_lisp_signal_hail(lone, machine, 1, tag, value);
        }

        actual = optional.value;

        /* Consume value... */

    case 1: /* Resumed via continuation... */
        partial = lone_lisp_machine_pop_value(lone, machine);
        actual = machine->value;

        /* Consume value... */

If no handler uses a continuation, the stack unwinds
past the primitive's frame and it is never resumed.

### Tags

The tag must not be nil. It is normally a symbol which
is matched by identity. However, applicable values are
also accepted: functions, primitives, continuations,
generators, vectors, tables. Callers can use them to
build elaborate tag discriminators. Symbol matchers
will not match an applicable tag. Function matchers
receive the tag as their argument and decide for
themselves. Nil tags are rejected at the call site.

### Values

The emitted `value` carries context that the handler can use.
The value could be anything, but the convention is to provide
handlers with the offending input, whenever meaningful.

 - `type-error`: the value that had the wrong type
 - `name-error`: the symbol that was not bound
 - `out-of-bounds`: the offending index as an integer
 - `arity-error`: the argument list (or its length as an integer)
 - `out-of-memory`: nil, or the attempted allocation size

Values can be arbitrarily rich: tables support any schema
a handler wants to consume.

## List of common errors

| Tag                   | Origin                                         |
|-----------------------|------------------------------------------------|
| `type-error`          | Unexpected or wrong value type                 |
| `arity-error`         | Wrong argument count to function or primitive  |
| `name-error`          | Unbound symbol lookup in the environment       |
| `key-error`           | Key not found in table                         |
| `integer-overflow`    | Caught integer overflow in math operations     |
| `division-by-zero`    | `/` with zero divisor                          |
| `out-of-memory`       | Memory allocator returned NULL                 |
| `out-of-bounds`       | List, vector, or bytes access past valid range |
| `syntax-error`        | Reader malformed input                         |
| `stack-overflow`      | Machine or generator stack hit its limit       |
| `inapplicable`        | Attempt to apply an inapplicable value         |

Tags are most commonly symbols. Call sites introduce new tags as needed.
The table above is merely guidance for common cases.
It is not an exhaustive list nor a closed set.

## The `signal` primitive

The lone lisp primitive `(signal tag value)`
and the C functions `cast` and `hail`
converge on the same dispatch code.
The functions apply the `signal` primitive:
the machine enters `APPLICATION` for the `signal`
primitive, whose step `0` destructures its arguments
and dispatches to the nearest matching `intercept`or.
Lisp level and C level signals follow the same path
from dispatch onward.
