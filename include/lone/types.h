#ifndef LONE_TYPES_HEADER
#define LONE_TYPES_HEADER

#include <stdbool.h>

#include <linux/types.h>

typedef __kernel_size_t size_t;
typedef __kernel_ssize_t ssize_t;

/* ╭──────────────────────────┨ LONE LISP TYPES ┠───────────────────────────╮
   │                                                                        │
   │    Lone implements dynamic data types as a tagged union.               │
   │    Supported types are:                                                │
   │                                                                        │
   │        ◦ Module       the isolated programming environment type        │
   │        ◦ Function     the reusable executable expressions type         │
   │        ◦ Primitive    the built-in C subroutine type                   │
   │        ◦ List         the linked list and tree type                    │
   │        ◦ Vector       the contiguous array of values type              │
   │        ◦ Table        the hash table, prototype and object type        │
   │        ◦ Symbol       the keyword and interned string type             │
   │        ◦ Text         the UTF-8 encoded text type                      │
   │        ◦ Bytes        the binary data and low level string type        │
   │        ◦ Integer      the signed integer type                          │
   │        ◦ Pointer      the memory addressing and dereferencing type     │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

enum lone_type {
	LONE_MODULE,
	LONE_FUNCTION,
	LONE_PRIMITIVE,
	LONE_LIST,
	LONE_VECTOR,
	LONE_TABLE,
	LONE_SYMBOL,
	LONE_TEXT,
	LONE_BYTES,
	LONE_INTEGER,
	LONE_POINTER,
};

enum lone_pointer_type {
	LONE_TO_UNKNOWN,

	LONE_TO_U8,  LONE_TO_I8,
	LONE_TO_U16, LONE_TO_I16,
	LONE_TO_U32, LONE_TO_I32,
	LONE_TO_U64, LONE_TO_I64,
};

struct lone_value;
struct lone_module;
struct lone_function;
struct lone_function_flags;
struct lone_list;
struct lone_vector;
struct lone_table;
struct lone_text;
struct lone_bytes;
struct lone_pointer;

struct lone_lisp;
struct lone_memory;

typedef bool (*lone_predicate)(struct lone_value *);
typedef bool (*lone_comparator)(struct lone_value *, struct lone_value *);

typedef struct lone_value *(*lone_primitive)(struct lone_lisp *lone,
                                             struct lone_value *module,
                                             struct lone_value *environment,
                                             struct lone_value *arguments,
                                             struct lone_value *closure);


#endif /* LONE_TYPES_HEADER */
