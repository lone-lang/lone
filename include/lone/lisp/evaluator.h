/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_EVALUATOR_HEADER
#define LONE_LISP_EVALUATOR_HEADER

#include <lone/types.h>

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

struct lone_value *lone_evaluate(
	struct lone_lisp *lone,
	struct lone_value *module,
	struct lone_value *environment,
	struct lone_value *value
);

struct lone_value *lone_evaluate_all(
	struct lone_lisp *lone,
	struct lone_value *module,
	struct lone_value *environment,
	struct lone_value *list
);

struct lone_value *lone_evaluate_module(
	struct lone_lisp *lone,
	struct lone_value *module,
	struct lone_value *value
);

struct lone_value *lone_apply(
	struct lone_lisp *lone,
	struct lone_value *module,
	struct lone_value *environment,
	struct lone_value *applicable,
	struct lone_value *arguments
);

#endif /* LONE_LISP_EVALUATOR_HEADER */
