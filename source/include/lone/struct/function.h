#ifndef LONE_STRUCT_FUNCTION_SOURCE_HEADER
#define LONE_STRUCT_FUNCTION_SOURCE_HEADER

#include <lone/types.h>

/* https://dl.acm.org/doi/10.1145/947941.947948
 * https://user.ceng.metu.edu.tr/~ucoluk/research/lisp/lispman/node24.html
 */
struct lone_function_flags {
	bool evaluate_arguments: 1;
	bool evaluate_result: 1;
	bool variable_arguments: 1;
};

struct lone_function {
	struct lone_value *arguments;        /* the bindings */
	struct lone_value *code;             /* the lambda */
	struct lone_value *environment;      /* the closure */
	struct lone_function_flags flags;    /* how to evaluate & apply */
};

#endif /* LONE_STRUCT_FUNCTION_SOURCE_HEADER */
