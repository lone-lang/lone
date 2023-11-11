/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_VALUE_FUNCTION_HEADER
#define LONE_VALUE_FUNCTION_HEADER

#include <lone/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone functions represent a body of executable lone lisp code.       │
   │    They have a list of argument names to be bound during function      │
   │    application, a list of expressions to be evaluated when called      │
   │    and a closure: a reference to the environment it was defined in.    │
   │                                                                        │
   │    To apply a function is to create a new environment with its         │
   │    argument names bound to the given arguments and then evaluate       │
   │    the function's expressions in the context of that environment.      │
   │                                                                        │
   │    The function flags control how the function is applied.             │
   │    It may be configured to receive evaluated or unevaluated            │
   │    arguments as well as to evaluate the result automatically.          │
   │    These features allow code manipulation and generation.              │
   │    It may also be configured to be variadic: all arguments             │
   │    are collected into a list and passed as a single argument.          │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_value *lone_function_create(
	struct lone_lisp *lone,
	struct lone_value *arguments,
	struct lone_value *code,
	struct lone_value *environment,
	struct lone_function_flags flags
);

#endif /* LONE_VALUE_FUNCTION_HEADER */
