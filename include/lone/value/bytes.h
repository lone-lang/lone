/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_VALUE_BYTES_HEADER
#define LONE_VALUE_BYTES_HEADER

#include <lone/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone bytes values can be created zero-filled for a given size       │
   │    or they can be initialized with a pointer to a memory block         │
   │    of known size.                                                      │
   │                                                                        │
   │    They can take ownership of arbitrary memory blocks via transfers    │
   │    or make copies of their input data.                                 │
   │                                                                        │
   │    Transferring memory blocks allows control over deallocation.        │
   │    Disabling deallocation on garbage collection allows pointing to     │
   │    data such as statically allocated buffers and C string literals.    │
   │    Enabling deallocation will cause the pointer to be deallocated      │
   │    when the bytes object is garbage collected. Two bytes objects       │
   │    cannot own the same memory block; it would lead to double free.     │
   │    This mode of operation is suitable for memory allocated by lone.    │
   │                                                                        │
   │    Copies will automatically include a hidden trailing null            │
   │    byte to ease compatibility with code expecting C strings.           │
   │    It's impossible to escape from them since system calls use them.    │
   │    Transferred buffers should also contain that null byte              │
   │    but the lone bytes type currently has no way to enforce this.       │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_value *lone_bytes_transfer(struct lone_lisp *lone, unsigned char *pointer, size_t count, bool should_deallocate);
struct lone_value *lone_bytes_transfer_bytes(struct lone_lisp *lone, struct lone_bytes bytes, bool should_deallocate);
struct lone_value *lone_bytes_copy(struct lone_lisp *lone, unsigned char *pointer, size_t count);
struct lone_value *lone_bytes_create(struct lone_lisp *lone, size_t count);

#endif /* LONE_VALUE_BYTES_HEADER */
