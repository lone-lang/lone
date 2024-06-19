/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_READER_HEADER
#define LONE_LISP_READER_HEADER

#include <lone/types.h>
#include <lone/lisp/types.h>

/* ╭─────────────────────────┨ LONE LISP READER ┠───────────────────────────╮
   │                                                                        │
   │    The reader's job is to transform input into lone lisp values.       │
   │    It accomplishes the task by reading input from a given file         │
   │    descriptor and then lexing and parsing the results.                 │
   │                                                                        │
   │    The lexer or tokenizer transforms a linear stream of characters     │
   │    into a linear stream of tokens suitable for parser consumption.     │
   │    This gets rid of insignificant whitespace and reduces the size      │
   │    of the parser's input significantly.                                │
   │                                                                        │
   │    It consists of an input buffer, its current position in it          │
   │    as well as two functions:                                           │
   │                                                                        │
   │        ◦ peek(k) which returns the character at i+k                    │
   │        ◦ consume(k) which advances i by k positions                    │
   │                                                                        │
   │    The parser transforms a linear sequence of tokens into a nested     │
   │    sequence of lisp objects suitable for evaluation.                   │
   │    Its main task is to match nested structures such as lists.          │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

void lone_lisp_reader_for_bytes(struct lone_lisp *lone, struct lone_lisp_reader *reader,
		struct lone_bytes bytes);

void lone_lisp_reader_for_file_descriptor(struct lone_lisp *lone, struct lone_lisp_reader *reader,
		size_t buffer_size, int file_descriptor);

void lone_lisp_reader_finalize(struct lone_lisp *lone, struct lone_lisp_reader *reader);

struct lone_lisp_value lone_lisp_read(struct lone_lisp *lone, struct lone_lisp_reader *reader);

#endif /* LONE_LISP_READER_HEADER */
