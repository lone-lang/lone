/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_EVALUATOR_HEADER
#define LONE_LISP_EVALUATOR_HEADER

#include <lone/lisp/types.h>

/* ╭────────────────────────┨ LONE LISP EVALUATOR ┠─────────────────────────╮
   │                                                                        │
   │    The heart of the language. This is what actually executes code.     │
   │                                                                        │
   │    Evaluator features:                                                 │
   │                                                                        │
   │        ◦ Symbol resolution                                             │
   │        ◦ Application of arguments                                      │
   │          ◦ Functions                                                   │
   │            ◦ Evaluated and unevaluated arguments                       │
   │            ◦ Evaluated and unevaluated result                          │
   │            ◦ Variadic                                                  │
   │          ◦ Primitives                                                  │
   │            ◦ Evaluated and unevaluated arguments                       │
   │            ◦ Evaluated and unevaluated result                          │
   │          ◦ Vectors                                                     │
   │            ◦ Indexing                                                  │
   │            ◦ Assignment                                                │
   │          ◦ Tables                                                      │
   │            ◦ Lookup                                                    │
   │            ◦ Assignment                                                │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_lisp_value lone_lisp_evaluate_in_module(
	struct lone_lisp *lone,
	struct lone_lisp_value module,
	struct lone_lisp_value value
);

#endif /* LONE_LISP_EVALUATOR_HEADER */
