/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_DEFINITIONS_HEADER
#define LONE_LISP_DEFINITIONS_HEADER

#include <lone/definitions.h>

#define LONE_LISP_DECIMAL_DIGITS_PER_INTEGER LONE_DECIMAL_DIGITS_PER_LONG

#ifndef LONE_LISP_BUFFER_SIZE
	#define LONE_LISP_BUFFER_SIZE 4096
#endif

#ifndef LONE_LISP_MEMORY_SIZE
	#define LONE_LISP_MEMORY_SIZE (1024 * 1024)
#endif

#ifndef LONE_LISP_HEAP_INITIAL_CAPACITY
	#define LONE_LISP_HEAP_INITIAL_CAPACITY (1024 * 1024)
#endif

#ifndef LONE_LISP_HEAP_GROWTH_FACTOR
	#define LONE_LISP_HEAP_GROWTH_FACTOR 2
#endif

#ifndef LONE_LISP_TABLE_LOAD_FACTOR_NUMERATOR
	#define LONE_LISP_TABLE_LOAD_FACTOR_NUMERATOR 70
#endif

#ifndef LONE_LISP_TABLE_LOAD_FACTOR_DENOMINATOR
	#define LONE_LISP_TABLE_LOAD_FACTOR_DENOMINATOR 100
#endif

/* required to be a power of 2 */
#define LONE_LISP_TABLE_GROWTH_FACTOR 2

#define LONE_LISP_PRIMITIVE(name)                       \
long lone_lisp_primitive_ ## name                       \
(                                                       \
	struct lone_lisp *lone,                         \
	struct lone_lisp_machine *machine,              \
	long step                                       \
)

#endif /* LONE_LISP_DEFINITIONS_HEADER */
