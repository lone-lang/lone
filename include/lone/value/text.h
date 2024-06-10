/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_VALUE_TEXT_HEADER
#define LONE_VALUE_TEXT_HEADER

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone texts are lone's strings and represent UTF-8 encoded text.     │
   │    Transfer and creation functions work like lone bytes.               │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_value lone_text_transfer(struct lone_lisp *lone, unsigned char *text, size_t length, bool should_deallocate);
struct lone_value lone_text_transfer_bytes(struct lone_lisp *lone, struct lone_bytes bytes, bool should_deallocate);
struct lone_value lone_text_copy(struct lone_lisp *lone, unsigned char *text, size_t length);
struct lone_value lone_text_from_c_string(struct lone_lisp *lone, char *c_string);
struct lone_value lone_text_to_symbol(struct lone_lisp *lone, struct lone_value text);

#endif /* LONE_VALUE_TEXT_HEADER */
