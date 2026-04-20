# Signal interception reentrancy

A matcher expression in an interceptor clause is evaluated
by the machine during signal dispatch. A handler expression
in an interceptor clause is evaluated by the machine after
its corresponding matcher expression has returned true.
The matcher and handler expressions may be arbitrary
lone lisp code, including code that emits a signal.

If either expression emits a signal, signal dispatch
is performed once again. Handling this edge case
requires that signal interception have some degree
of reentrancy.

A naive implementation would walk up the stack and
find the same interceptor whose clauses are currently
being evaluated. It would layer new state on top of
the half-unwound stack and corrupt control flow.
Lone will do the right thing.

The rule is:

A signal emitted during another signal's dispatch,
whether during stack walking or the evaluation of
matcher or handler expressions, must propagate outward
past every interceptor already involved in the dispatch.

This lets matcher bodies compose error signals freely.
A matcher that wraps the original tag and value in a
richer structure and re-signals under a different tag
just works.

    (intercept

      (('translator (lambda (e)
                      (signal 'wrapped
                        { original e from 'matcher }))))

      (code-that-emits 'translator))

## Mechanism

The `INTERCEPTOR_DELIMITER` tag `0x45` is odd-tagged in
the stack-frame tag scheme, meaning its data bits carry
no heap reference and are free for auxiliary use.
The bit just above the 8-bit tag byte is reserved
as a `dispatching` flag.

    LONE_LISP_INTERCEPTOR_DISPATCHING_FLAG = 1 << 8

`lone_lisp_signal_find_interceptor_delimiter` walks
downward from `stack.top`. At each `INTERCEPTOR_DELIMITER`
it encounters, it tests the dispatching flag. If set,
the walker skips the delimiter and keeps searching.
If clear, it returns the delimiter as the match.

## Dispatching flag life cycle

The walker sets the flag on each delimiter whose clauses
it evaluates. If no clause matches, the flag stays set
and the walker continues outward to the next delimiter.
This builds a monotonically outward chain: interceptors
the dispatcher has already visited are excluded from
subsequent nested dispatches triggered by matchers
or handlers further up the chain. In concrete terms,
emitted signals can only ever propagate outwards.

The flag is cleared or destroyed in two paths.
The arity-2 handler path captures stack frames
into a delimited continuation and explicitly
clears the dispatching bits on all interceptor
delimiters before capture. The arity-1 handler
path truncates the stack past the delimiter,
destroying the frame and the flag along with it.

## Interaction with continuation capture

The stack range captured by the arity-2 handler is
`[delimiter - 2, stack.top - 1)` in frame pointers,
after the values pushed during dispatch have been
popped. This range includes the `INTERCEPTOR_DELIMITER`
itself and any inner interceptor delimiters that were
visited and flagged during the dispatch.

Before `lone_memory_move` copies the range to the
heap-allocated stack frame array, the dispatching
flag is cleared on every `INTERCEPTOR_DELIMITER`
in the captured range. This is the only code path
that clears this flag.

If the flags were left set, `push_frames` would restore
delimiters with `dispatching=1`. The walker would skip
every restored interceptor for any subsequent emission,
even though the original dispatch has concluded and the
code is resuming fresh. Clearing the flags before capture
makes all restored interceptors valid candidates for new
signals emitted from the post-resume code, since the
intercept forms are being re-entered in a fresh context.
This is intuitive for the users of the language.

## Interaction with generator termination

Delimiter frames on generator stacks are processed
as described above. The only interesting interaction
happens when a generator boundary is crossed during
signal dispatch.

When that happens, `signal_dispatch` terminates the
generator which destroys all frames in the generator's
own stack, including all delimiter frames on it.

## Bounded propagation

An interceptor's matcher that signals without a matching
outer interceptor falls through to the system interceptor
which currently simply exits the program. A matcher that
signals in a way that could itself be caught only by the
currently-in-flight interceptor is rejected for the same
reason: each dispatching interceptor is permanently out
of the candidate set for the duration of the dispatch,
so no loop can form. Cascades are bounded by the stack
depth of enclosing interceptors.

## Tag space considerations

`LONE_LISP_TAG_MASK` is `0xFF`: the tag occupies the low
8 bits of the 64-bit tagged word. Data bits start at bit
8. All existing code that inspects an `INTERCEPTOR_DELIMITER`
frame uses the tag mask and ignores the data bits, so the
dispatching flag is invisible to code that doesn't know
about it. The garbage collector treats `INTERCEPTOR_DELIMITER`
as a register value and does not scan its data bits.
