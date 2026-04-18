/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_DEFINITIONS_HEADER
#define LONE_LISP_DEFINITIONS_HEADER

#include <lone/definitions.h>

#define LONE_LISP_DECIMAL_DIGITS_PER_INTEGER LONE_DECIMAL_DIGITS_PER_LONG

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Tagged value layout constants.                                      │
   │                                                                        │
   │    All lone lisp values are 64-bit tagged words.                       │
   │    The low byte is the tag. The upper 56 bits are data.                │
   │                                                                        │
   │    For non-heap values with bit 0 = 1, all 56 bits represent data.     │
   │                                                                        │
   │        63                            8  7      0                       │
   │        ┌──────────────────────────────┬─────────┐                      │
   │        │  data (56)                   │ tag (8) │                      │
   │        └──────────────────────────────┴─────────┘                      │
   │                                                                        │
   │    For heap values with bit 0 = 0, the data is further split           │
   │    into 16 bits of per-value metadata and a 40-bit heap index.         │
   │                                                                        │
   │        63              24  23        8  7      0                       │
   │        ┌─────────────────┬────────────┬─────────┐                      │
   │        │  index (40)     │ meta (16)  │ tag (8) │                      │
   │        └─────────────────┴────────────┴─────────┘                      │
   │                                                                        │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

#define LONE_LISP_TAG_BITS              8
#define LONE_LISP_TAG_MASK              0xFF

#define LONE_LISP_DATA_BITS             56
#define LONE_LISP_DATA_SHIFT            LONE_LISP_TAG_BITS

#define LONE_LISP_METADATA_BITS         16
#define LONE_LISP_METADATA_SHIFT        LONE_LISP_TAG_BITS
#define LONE_LISP_METADATA_MASK         0xFFFF

#define LONE_LISP_INDEX_BITS            40
#define LONE_LISP_INDEX_SHIFT           24

/* FEXPR flags in metadata for functions and primitives.
 * These are bit positions within the full tagged word,
 * allowing direct bit tests without extraction.
 */
#define LONE_LISP_METADATA_EVALUATE_ARGUMENTS  (1L << 8)
#define LONE_LISP_METADATA_EVALUATE_RESULT     (1L << 9)

/* Function arity in metadata bits 10-13.
 * 4 bits = 0-14.
 * Value 15 = overflow sentinel: 15+, check heap structure.
 * Shift and mask extract the arity from the full tagged word.
 */
#define LONE_LISP_METADATA_ARITY_SHIFT    10
#define LONE_LISP_METADATA_ARITY_MASK     0x0F
#define LONE_LISP_METADATA_ARITY_OVERFLOW 0x0F

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Inline small values in lisp values as constants.                    │
   │                                                                        │
   │    Values with data ≤ 7 bytes are encoded directly                     │
   │    in the lisp value tagged word, avoiding heap allocation.            │
   │                                                                        │
   │    Tag byte layout:                                                    │
   │                                                                        │
   │        bit 7 = 1: inline flag                                          │
   │        bits 4-6:  inline type                                          │
   │        bits 1-3:  length                                               │
   │        bit 0 = 1: register value flag                                  │
   │                                                                        │
   │    Inline value types:                                                 │
   │                                                                        │
   │        000 = symbol                                                    │
   │        001 = text                                                      │
   │        010 = bytes                                                     │
   │                                                                        │
   │    Data layout:                                                        │
   │                                                                        │
   │      63          16  15       8  7      0                              │
   │      ┌──────────────┬──────────┬─────────┐                             │
   │      │  bytes 1-6   │  byte 0  │  tag    │                             │
   │      └──────────────┴──────────┴─────────┘                             │
   │                                                                        │
   │    Bytes are stored starting at bit 8.                                 │
   │    Unused high bytes are zero.                                         │
   │    Two inline values with the same content                             │
   │    produce identical tagged words.                                     │
   │    Identity comparison works.                                          │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

#define LONE_LISP_INLINE_FLAG              0x80
#define LONE_LISP_INLINE_TYPE_MASK         0xF1  /* bit 7 + bits 4-6 + bit 0 */
#define LONE_LISP_INLINE_TYPE_SYMBOL       0x81  /* 1_000_xxx_1 */
#define LONE_LISP_INLINE_TYPE_TEXT         0x91  /* 1_001_xxx_1 */
#define LONE_LISP_INLINE_TYPE_BYTES        0xA1  /* 1_010_xxx_1 */
#define LONE_LISP_INLINE_LENGTH_SHIFT      1
#define LONE_LISP_INLINE_LENGTH_MASK       0x07  /* 3 bits for length 0-7 */
#define LONE_LISP_INLINE_MAX_LENGTH        7

/* Flag bit in an INTERCEPTOR_DELIMITER stack frame.
 * Set while that interceptor is dispatching a signal.
 * The stack walker skips dispatching delimiters so that
 * a signal emitted from a matcher expression propagates
 * to the next interceptor rather than reentering the
 * current one.
 */
#define LONE_LISP_INTERCEPTOR_DISPATCHING_FLAG \
	((long) 1 << LONE_LISP_DATA_SHIFT)

#ifndef LONE_LISP_BUFFER_SIZE
	#define LONE_LISP_BUFFER_SIZE 4096
#endif

#ifndef LONE_LISP_MEMORY_SIZE
	#define LONE_LISP_MEMORY_SIZE (1024 * 1024)
#endif

#ifndef LONE_LISP_HEAP_INITIAL_CAPACITY
	#define LONE_LISP_HEAP_INITIAL_CAPACITY (1024 * 1024)
#endif

#define LONE_LISP_TABLE_INDEX_EMPTY ((size_t) -1)

#ifndef LONE_LISP_HEAP_GROWTH_FACTOR
	#define LONE_LISP_HEAP_GROWTH_FACTOR 2
#endif

#ifndef LONE_LISP_MACHINE_STACK_INITIAL_SIZE
	#define LONE_LISP_MACHINE_STACK_INITIAL_SIZE 256
#endif

#ifndef LONE_LISP_GENERATOR_STACK_INITIAL_SIZE
	#define LONE_LISP_GENERATOR_STACK_INITIAL_SIZE 128
#endif

#ifndef LONE_LISP_MACHINE_STACK_MAXIMUM_SIZE
	#define LONE_LISP_MACHINE_STACK_MAXIMUM_SIZE 65536
#endif

#ifndef LONE_LISP_MACHINE_STACK_GROWTH_FACTOR
	#define LONE_LISP_MACHINE_STACK_GROWTH_FACTOR 2
#endif

#ifndef LONE_LISP_MACHINE_STACK_SHRINK_THRESHOLD
	#define LONE_LISP_MACHINE_STACK_SHRINK_THRESHOLD 8
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

#define LONE_LISP_INTEGER_MIN (-(1L << (LONE_LISP_DATA_BITS - 1)))
#define LONE_LISP_INTEGER_MAX ((1L << (LONE_LISP_DATA_BITS - 1)) - 1)

#endif /* LONE_LISP_DEFINITIONS_HEADER */
