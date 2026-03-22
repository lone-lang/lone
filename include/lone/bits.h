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
   │    to its real contents.                                               │
   │                                                                        │
   │    Bit indexing order is MSB first.                                    │
   │    Bit 0 is the most significant bit of byte 0.                        │
   │    Bit 7 is the least significant bit of byte 0.                       │
   │    Bit 8 is the most significant bit of byte 1...                      │
   │                                                                        │
   │    The find functions support unaligned access                         │
   │    but are most efficient when given word aligned data.                │
   │    For maximum performance, ensure the bits pointer                    │
   │    is aligned to sizeof(unsigned long).                                │
   │    Unaligned leading and trailing bytes                                │
   │    are accessed byte-wise.                                             │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Returns the index-th bit from the given bits.                       │
   │    Bounds are not checked.                                             │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
bool lone_bits_get(void *bits, lone_size index);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Sets the index-th bit of the given bits to bit and returns it.      │
   │    Bounds are not checked.                                             │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
bool lone_bits_set(void *bits, lone_size index, bool bit);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Sets the index-th bit to one or zero respectively.                  │
   │    Bounds are not checked.                                             │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
void lone_bits_mark(void *bits, lone_size index);
void lone_bits_clear(void *bits, lone_size index);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Finds the index of the first set bit.                               │
   │    Size is total number of bytes that the bits span.                   │
   │    Returns an absent optional if no set bit was found.                 │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
struct lone_optional_size lone_bits_find_first_one(const void * restrict bits, lone_size size);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Finds the index of the first clear bit.                             │
   │    Size is total number of bytes that the bits span.                   │
   │    Returns an absent optional if no clear bit was found.               │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
struct lone_optional_size lone_bits_find_first_zero(const void * restrict bits, lone_size size);

#endif /* LONE_BITS_HEADER */
