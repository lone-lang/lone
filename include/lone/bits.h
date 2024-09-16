/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_BITS_HEADER
#define LONE_BITS_HEADER

#include <lone/types.h>

/* ╭──────────────────────────┨ LONE BITS ┠─────────────────────────────────╮
   │                                                                        │
   │    Lone's bitmap/bitset implementation.                                │
   │                                                                        │
   │    Lone bits are idealized as a single contiguous block of memory      │
   │    of arbitrary contents, size and alignment. The bits functions       │
   │    simply treat this memory as an array of bits and are oblivious      │
   │    to its real contents. They may attempt to take advantage of         │
   │    pointer alignment in order to speed up operations.                  │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Returns the index-th bit from the given bits.                       │
   │    Bit indexing order is left to right. Bounds are not checked.        │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
bool lone_bits_get(void *bits, lone_size index);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Sets the index-th bit of the given bits to bit and returns it.      │
   │    Bit indexing order is left to right. Bounds are not checked.        │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
bool lone_bits_set(void *bits, lone_size index, bool bit);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Finds the index of the first set bit.                               │
   │    Bit indexing order is left to right.                                │
   │    Size is total number of bytes that the bits span.                   │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
lone_size lone_bits_find_first_zero(const void * restrict bits, lone_size size);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Finds the index of the first clear bit.                             │
   │    Bit indexing order is left to right.                                │
   │    Size is total number of bytes that the bits span.                   │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
lone_size lone_bits_find_first_one(const void * restrict bits, lone_size size);

#endif /* LONE_BITS_HEADER */
