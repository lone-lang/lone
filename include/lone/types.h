#ifndef LONE_TYPES_HEADER
#define LONE_TYPES_HEADER

#include <stdbool.h>

#include <linux/types.h>

typedef __kernel_size_t size_t;
typedef __kernel_ssize_t ssize_t;

struct lone_lisp;

struct lone_value;
struct lone_function;
struct lone_function_flags;
struct lone_bytes;
struct lone_list;
struct lone_vector;
struct lone_table;
struct lone_pointer;

enum lone_pointer_type {
	LONE_TO_UNKNOWN,

	LONE_TO_U8,  LONE_TO_I8,
	LONE_TO_U16, LONE_TO_I16,
	LONE_TO_U32, LONE_TO_I32,
	LONE_TO_U64, LONE_TO_I64,
};

struct lone_memory;

typedef bool (*lone_predicate)(struct lone_value *);
typedef bool (*lone_comparator)(struct lone_value *, struct lone_value *);

typedef struct lone_value *(*lone_primitive)(struct lone_lisp *lone,
                                             struct lone_value *module,
                                             struct lone_value *environment,
                                             struct lone_value *arguments,
                                             struct lone_value *closure);


#endif /* LONE_TYPES_HEADER */
