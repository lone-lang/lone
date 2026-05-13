# Shapes

Shaped table optimization
for lone lisp tables.
Collapses hash table lookups
into direct array indexing.

## Overview

Tables in lone serve as both hash maps and objects.
Environments are tables. Prototypal inheritance
is the prototype chain. Every access is a probe.
This is a general solution but in many cases
not the most efficient solution.

Luckily, certain patterns exist and can be exploited.
Tables created the same way end up with the same keys
in the same order. Every invocation of `(lambda (x y) ...)`
creates an environment with two keys: `x`, and `y`.
It creates a shape. Lone's tables are now shaped.

A **shape** is a special lone value that describes
which keys a table has and at which index they are.
A shaped table stores values in a flat array.
Lookup becomes a direct array indexing operation
instead of a sequence of hash, probe and compare
operations.

This is the same optimization employed by:

  - Self: shapes
  - V8: hidden classes
  - SpiderMonkey: shapes

## Motivation

Lone is paying significant costs for the general solution.

### Allocation costs per call before shapes

An environment table is created for every function application.
Before shapes, this meant allocating at least:

| Component     | Size          | Purpose                                          |
|---------------|---------------|--------------------------------------------------|
| Heap value    | 64 bytes      | The table object                                 |
| indexes array | 16 bytes      | At least 2 × `size_t` hash-to-entry map          |
| entries array | 32 bytes      | At least 2 × `(key, value)` pairs                |
| **Total**     | **112 bytes** | Environment that often holds only a few bindings |

Plus `memset` of the entire indexes array to `0xFF`
to mark all slots as `LONE_LISP_TABLE_INDEX_EMPTY`.

For a recursive `(fibonacci 30)` function with around 2.7 million calls:
at least 112 MB allocated and memset for environments with a single
variable binding.

Capacity 2 is the minimum enforced by the implementation.
The actual default was 16, making these numbers 4-5 times
worse in practice.

### Lookup cost per variable access before shapes

Function environments are lone lisp tables.
Every variable lookup by symbol would:

  1. Hash the key
  2. Probe the indexes array
  3. Compare key, structurally if needed
  4. If not found, recurse into prototype and restart

For `(fibonacci 30)`:

Looking up `n` from inside the function body: hash, probe, hit.
Looking up `fibonacci` from inside the function body:
hash, probe, miss since there's only `n` here,
recurse to prototype, re-hash, probe, hit.

## Shapes

A shape is a heap value that describes
a fixed set of keys and their positions.

```c
struct lone_lisp_shape {
    size_t count;
    struct lone_lisp_value *keys;
};
```

`count` is the number of keys in the shape.
`keys` is a dynamically allocated array
of `count` tagged values. They hold the
shape's keys in order.

No `prototype` or `transitions` fields.
Transition caching is deferred.

## Shaped tables

A table may be either a hash table or a shaped table.
The `shaped` bit on `lone_lisp_heap_value` distinguishes them.

```c
struct lone_lisp_table {
    size_t count;
    size_t capacity;

    union {
        struct {
            size_t used;
            size_t *indexes;
            struct lone_lisp_table_entry *entries;
        } hash;

        struct {
            struct lone_lisp_value shape;
            struct lone_lisp_value *values;
        } shaped;
    };

    struct lone_lisp_value prototype;
};
```

A shaped table stores only the `values` array.

For a function with a single parameter:

| Component    | Size         |
|--------------|--------------|
| Heap value   | 64 bytes     |
| values array | 8 bytes      |
| **Total**    | **72 bytes** |

**1.6× less allocation** at minimum capacity, even more in practice.

## Shapes are determined at lambda creation time

Functions are immutable. Their parameter lists are set in stone
after they are created. The shapes of their environments are
derived from those lists.

Functions with nil shapes simply fall back to normal hash tables.
The shapes are nil in case of:

  - Variadic functions: parameters end with `(rest)`
  - Zero argument functions: nothing to index into
  - Functions with more than 15 parameters

## Shaped argument binding

When the function has a non-nil shape, it either creates
a new shaped environment table or reuses the caller's table
if possible, then fills it directly from the arguments list.
Reuse is possible when in tail position and when the caller's
shape matches the current shape.

## Shaped table lookup

Simple linear scans.

```c
for (i = 0; i < shape->count; ++i) {
    if (lone_lisp_table_shape_key_matches(lone, shape->keys[i], key)) {
        return actual->shaped.values[i];
    }
}
```

One tagged word comparison per key for symbols.
Falls through to structural comparison for list,
text, and bytes keys. On miss, the prototype chain
is consulted normally.

## Deoptimization

Set operations on a shaped table check
whether the key is already in the shape.
If so, the value is simply updated in place.

If the key is not in the shape,
the table is **deoptimized**
to a normal hash table:

  1. Allocate `indexes` and `entries` arrays.
  2. Insert all shaped key/value pairs.
  3. Clear the `shaped` flag.
  4. Insert the new key.

This essentially converts the table
from a shaped table representation
to a hash table representation.
Swaps one union arm for the other.

Deoptimization is also triggered by deletion
which is a fully supported but somewhat rare
operation.

Deoptimization is a one time cost.
Tables that behave predictably
should never deoptimize.

## Environment reuse in tail position

When a function call is in tail position
and the current environment has the same
shape, the values array is overwritten
in place.

For tail-recursive Fibonacci, accumulator style:

```lisp
(set fibonacci (lambda (n a b)
  (if (< n 1) a
      (fibonacci (- n 1) b (+ a b)))))
```

Each recursive call overwrites `values[0..2]` in place.
Zero allocation per iteration.

## Interaction with the garbage collector

Shapes are values that get marked, swept and compacted
like any other. Shapes proliferate and are collected
naturally as the program runs.

Shapes have their keys array marked,
while shaped tables have their values
array marked. Dead shapes have their
keys array deallocated, and shaped
tables have their values array
deallocated.

Shape references in tables and function values
are forwarded by the compactor. Values inside
shaped tables' `values` arrays are forwarded.
Shape keys are forwarded.

## Performance

Approximately 5-6% faster than `master @ 6371a81f`
at executing (fibonacci 30) when compiled with
link time optimization and optimization level 2.

Primary gains: allocation reduction and elimination
of hashing for local variable lookups.

## Future work

### Shape transitions

Caching the result of adding a new key to a shaped table
so that subsequent invocations follow the same transition
chain rather than always deoptimizing.

### Inline cache

Caching symbol lookup results so repeated lookups
skip the key scan and prototype chain traversal.
A limited form of per-shape inline caches was
prototyped and resulted in dramatic 20% improvements.
However, they conflicted with shape sharing and transitions.
The correct architecture requires control over call sites,
which requires bytecode compilation and interpretation.
The optimization is deferred until lone has gained
the required bytecode features.
