# Emitting signals from primitives

The `(signal tag value)` lisp primitive emits a signal
that `(intercept clauses body...)` can intercept.
However, it only works at the lisp level.
A function to signal errors from C code
is also necessary. This document describes
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

Lone primitives emit signals through a single function:

    long
    lone_lisp_signal_emit(struct lone_lisp *lone,
                          struct lone_lisp_machine *machine,
                          long step,
                          struct lone_lisp_value tag,
                          struct lone_lisp_value value);

It sets `machine->applicable` to the `signal` primitive
and `machine->list` to the `(tag value)` argument list.
The return value must be returned by the calling primitive
to the machine. The `step` parameter selects between two
control-flow modes: abandon and resume.

## Abandoning

Passing `-2` causes the machine to tail apply the `signal`
primitive in the caller's context. The calling primitive's
step frame is discarded along with any partial state it
had pushed to the stack. The primitive cannot be resumed.
If signal captures a continuation and it is later called,
it will cause the primitive itself to return the value
passed to the continuation.

Example of tail apply error signaling:

    if (!lone_lisp_is_integer(lone, index)) {
        return lone_lisp_signal_emit(lone, machine, -2, type_error, index);
    }

Example that uses optional:

    optional = succeed_or_fail(argument);

    if (!optional.present) {
        return lone_lisp_signal_emit(lone, machine, -2, error, context);
    }

    actual = optional.value;

    /* Consume value... */

Use `-2` when the primitive has no meaningful way to continue:
a type mismatch that cannot be corrected from the handler's reply,
or an unrecoverable invariant violation.

### Resuming

Passing a positive step value makes the machine
save the calling primitive's step frame before
applying `signal`. If an interceptor's handler
captures a continuation, lisp code can, at any
later point, attempt to resume the primitive's
execution by calling the continuation. This will
resume the primitive at the specified step number,
and the value passed to the continuation will be
stored in `machine->value`. This enables primitives
to recover from errors when the handler provides a
substitute value.

    case 0:
    validate_and_consume:

        optional = succeed_or_fail(argument);

        if (!optional.present) {
            lone_lisp_machine_push_value(lone, machine, partial);
            return lone_lisp_signal_emit(lone, machine, 1, tag, value);
        }

        actual = optional.value;

        /* Consume value... */

    case 1: /* Resumed via continuation... */

        partial = lone_lisp_machine_pop_value(lone, machine);
        actual = machine->value;

        goto validate_and_consume;

If no handler uses a continuation, the primitive is simply
never resumed, and any state the primitive pushed before
emitting the signal is discarded along with all the frames.

A typical recovery pattern:

 1. Allocates the next unused step number for the resume case
 2. Saves any primitive state on the stack before emitting the signal
 3. Has the resume case unpack that state plus `machine->value`
    3.1. Then `goto`s the original code to revalidate the substitute

    case 0:

        argument = pop_value();

    validate:

        if (not_acceptable(argument)) {
            return lone_lisp_signal_emit(lone, machine, 1, tag, argument);
        }

        /* Proceed with argument... */

        return 0;

    case 1:

        argument = machine->value;

        goto validate;

If the value the primitive was resumed with is also unacceptable,
the validation resignals in the exact same way. The pattern handles
both successful recovery and repeated failure uniformly.

### The step argument

Currently, the `step` argument must be either `-2`
or a positive integer. The values `0`, `-1`,
and other negatives produce undefined behavior.

`0` would be interpreted as "primitive done, result on stack top".
Negatives other than `-2` would be interpreted as tail return with
`machine->expression`. Neither is meaningful for signal emission.
A `LONE_DEBUG` build rejects invalid steps at the call site.

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
and the C function `lone_lisp_signal_emit`
converge on the same dispatch code.
The function applies the `signal` primitive:
the machine enters `APPLICATION` for the `signal`
primitive, whose step `0` destructures its arguments
and dispatches to the nearest matching `intercept`or.
Lisp level and C level signals follow the same path
from dispatch onward.
