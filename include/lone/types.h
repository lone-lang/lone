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

struct lone_memory;

typedef bool (*lone_predicate)(struct lone_value *);
typedef bool (*lone_comparator)(struct lone_value *, struct lone_value *);

typedef struct lone_value *(*lone_primitive)(struct lone_lisp *lone,
                                             struct lone_value *module,
                                             struct lone_value *environment,
                                             struct lone_value *arguments,
                                             struct lone_value *closure);


#endif /* LONE_TYPES_HEADER */
