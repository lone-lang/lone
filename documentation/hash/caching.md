# Value hash caching

Every hashable lone lisp value carries its own hash.
The hash is computed once per value lifetime and reused
for every subsequent operation. Once the cache is hot,
table lookups never need to invoke hash functions again.

## Overview

The hashing operation is expensive, so its cost is paid once
and amortized across every subsequent query. Two paths handle
the two value storage forms.

 - **Inline values**

   Integers, inline symbols, inline texts, inline bytes.
   The hash function is the identity function.
   The tagged word itself is the hash.
   Two equal inline values are identical
   and therefore hash equally.

 - **Heap values**

   Heap symbols, heap texts, heap bytes, lists.
   The hash lives in a dedicated field within
   each hashable type's structure.
   The `hash_cached` bit in the heap value's
   flags structure indicates whether a hash
   has been calculated and cached.

## Storage layout

Each hashable heap type carries a `hash` field:

    struct lone_lisp_symbol {
        struct lone_bytes name;
        lone_hash hash;
    };

    struct lone_lisp_text {
        struct lone_bytes bytes;
        lone_hash hash;
    };

    struct lone_lisp_bytes {
        struct lone_bytes data;
        lone_hash hash;
    };

    struct lone_lisp_list {
        struct lone_lisp_value first;
        struct lone_lisp_value rest;
        lone_hash hash;
    };

It's structured so that all of them have the same offset.
This is statically asserted at compile time.

Code reaches the field through a dispatch function
that selects the correct union member by type tag.

The `hash_cached` flag lives in the flags structure
alongside `frozen` and `should_deallocate_bytes`.
The flag is set when the hash is calculated
and stored for the first time. The allocator
clears it when allocating new heap values.

## Hash computation timing

| Type        | Timing            | Trigger                         |
|-------------|-------------------|---------------------------------|
| Heap symbol | Eager             | Symbol intern                   |
| Heap text   | Lazy              | First `lone_lisp_hash_of` call  |
| Heap bytes  | Lazy after freeze | First call after the freeze bit |
| Heap list   | Lazy, recurses    | First call; combines children   |

Symbols are nearly always used as keys,
so they are born with pre-computed hashes.

Text, bytes, and lists are less common keys
and pay only on first hash query. Lists recurse
through `lone_lisp_hash_of` so each cell caches
on the way back up. Any sublist hashed thereafter
hits the cache in constant time.

## The hash function

The hash caching architecture is agnostic
to the actual hash function being used.
It just caches the result of whatever
function is in use at the time.

Inline values use the identity function.
Other values currently use SipHash-2-4
with a 128-bit key seeded with entropy
from the auxiliary vector's `AT_RANDOM`
bytes.

## Lookup

The public entry point:

    lone_hash lone_lisp_hash_of(struct lone_lisp *lone,
                                struct lone_lisp_value value);

Inline values return the tagged word directly.

Heap values check `hash_cached`. On a hit, the cached hash
is read through the dispatch helper. On a miss, control
passes to the helper:

    lone_hash
    lone_lisp_value_compute_and_store_hash(struct lone_lisp *lone,
                                           struct lone_lisp_value value);

The helper dispatches by type, computes the hash
via the configured function, stores the result,
and sets `hash_cached` to true.

Symbols, texts, and bytes longer than seven bytes hash by content.
Bytes of seven bytes or fewer hash to the equivalent inline form's
tagged word, so heap and inline bytes of identical content always
produce identical hashes. Lists combine the cached hashes of their
`first` and `rest` children with the `LIST` tag prepended.

## Equality and the hash invariant

Equal values must produce equal hashes. The architecture
preserves the invariant across every form a hashable value
can take.

 - **Symbols**

   Symbols are interned. A given name has exactly one
   canonical form: inline if seven bytes or fewer, heap
   otherwise. Equal symbols share a tagged word and
   therefore hash equally.

 - **Texts**

   Texts of seven bytes or fewer are always inline.
   Heap text only exists for content longer than
   seven bytes. The two forms never overlap,
   so they never need to hash equally.

 - **Bytes**

   Inline bytes are always frozen and use their tagged
   word as the hash. Heap bytes are mutable until they
   are explicitly frozen, and only then are they hashable.
   A frozen heap bytes value of seven bytes or fewer can
   hold the same content as an inline bytes value.
   To keep their hashes equal, the short heap bytes
   synthesize the equivalent inline tagged word at
   hash time.

   Heap bytes longer than seven bytes have no inline
   counterpart and hash by content.

 - **Lists**

   Lists are always heap allocated. Two lists are equal
   when their structures recurse to equal children
   at every step. Their cached hashes combine
   the same children's hashes through
   the same function.

 - **Integers and singletons**

   Integers and the singletons `nil`, `true`, `false`
   have unique tagged words for each value. The tagged
   word is canonical and serves as the hash.

## Probe pre-filter

Hash table probes short-circuit on tagged-word equality.
Exact for values with a single canonical tagged word:
interned symbols, integers, the singletons, and inline
values that compare equal by content.

For probes on `LIST`, `TEXT`, or `BYTES` keys
whose tagged words differ from the stored entry's,
the probe compares the two cached hashes before
falling through to structural comparison.

Equal values always produce the same hash,
so a hash mismatch is a definitive negative.
Structural comparison is only performed when
the hashes agree. Tables keyed by lists, texts,
or bytes skip the structural compare on every
probed mismatch.
