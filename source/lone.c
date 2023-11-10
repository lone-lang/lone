/* SPDX-License-Identifier: AGPL-3.0-or-later */

/* ╭─────────────────────────────┨ LONE LISP ┠──────────────────────────────╮
   │                                                                        │
   │                       The standalone Linux Lisp                        │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
#include <stdint.h>

#include <lone/definitions.h>
#include <lone/types.h>
#include <lone/structures.h>
#include <lone/constants.h>
#include <lone/hash.h>
#include <lone/value.h>
#include <lone/value/module.h>
#include <lone/value/function.h>
#include <lone/value/primitive.h>
#include <lone/value/bytes.h>
#include <lone/value/text.h>
#include <lone/value/symbol.h>
#include <lone/value/list.h>
#include <lone/value/vector.h>
#include <lone/value/table.h>
#include <lone/value/integer.h>
#include <lone/value/pointer.h>
#include <lone/memory.h>
#include <lone/linux.h>
#include <lone/lisp.h>
#include <lone/lisp/reader.h>
#include <lone/lisp/evaluator.h>
#include <lone/lisp/printer.h>
#include <lone/utilities.h>
#include <lone/modules.h>
#include <lone/modules/lone.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Built-in mathematical and numeric operations.                       │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_primitive_integer_operation(struct lone_lisp *lone, struct lone_value *arguments, char operation, long accumulator)
{
	struct lone_value *argument;

	if (lone_is_nil(arguments)) { /* wasn't given any arguments to operate on: (+), (-), (*) */ goto return_accumulator; }

	do {
		argument = lone_list_first(arguments);
		if (!lone_is_integer(argument)) { /* argument is not a number */ linux_exit(-1); }

		switch (operation) {
		case '+': accumulator += argument->integer; break;
		case '-': accumulator -= argument->integer; break;
		case '*': accumulator *= argument->integer; break;
		default: /* invalid primitive integer operation */ linux_exit(-1);
		}

		arguments = lone_list_rest(arguments);

	} while (!lone_is_nil(arguments));

return_accumulator:
	return lone_integer_create(lone, accumulator);
}

static struct lone_value *lone_primitive_add(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	return lone_primitive_integer_operation(lone, arguments, '+', 0);
}

static struct lone_value *lone_primitive_subtract(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *first;
	long accumulator;

	if (!lone_is_nil(arguments) && !lone_is_nil(lone_list_rest(arguments))) {
		/* at least two arguments, set initial value to the first argument: (- 100 58) */
		first = lone_list_first(arguments);
		if (!lone_is_integer(first)) { /* argument is not a number */ linux_exit(-1); }
		accumulator = first->integer;
		arguments = lone_list_rest(arguments);
	} else {
		accumulator = 0;
	}

	return lone_primitive_integer_operation(lone, arguments, '-', accumulator);
}

static struct lone_value *lone_primitive_multiply(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	return lone_primitive_integer_operation(lone, arguments, '*', 1);
}

static struct lone_value *lone_primitive_divide(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *dividend, *divisor;

	if (lone_is_nil(arguments)) { /* at least the dividend is required, (/) is invalid */ linux_exit(-1); }
	dividend = lone_list_first(arguments);
	if (!lone_is_integer(dividend)) { /* can't divide non-numbers: (/ "not a number") */ linux_exit(-1); }
	arguments = lone_list_rest(arguments);

	if (lone_is_nil(arguments)) {
		/* not given a divisor, return 1/x instead: (/ 2) = 1/2 */
		return lone_integer_create(lone, 1 / dividend->integer);
	} else {
		/* (/ x a b c ...) = x / (a * b * c * ...) */
		divisor = lone_primitive_integer_operation(lone, arguments, '*', 1);
		return lone_integer_create(lone, dividend->integer / divisor->integer);
	}
}

static struct lone_value *lone_primitive_is_less_than(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	return lone_apply_comparator(lone, arguments, lone_integer_is_less_than);
}

static struct lone_value *lone_primitive_is_less_than_or_equal_to(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	return lone_apply_comparator(lone, arguments, lone_integer_is_less_than_or_equal_to);
}

static struct lone_value *lone_primitive_is_greater_than(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	return lone_apply_comparator(lone, arguments, lone_integer_is_greater_than);
}

static struct lone_value *lone_primitive_is_greater_than_or_equal_to(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	return lone_apply_comparator(lone, arguments, lone_integer_is_greater_than_or_equal_to);
}

static struct lone_value *lone_primitive_sign(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *value;
	if (lone_is_nil(arguments)) { /* no arguments: (sign) */ linux_exit(-1); }
	value = lone_list_first(arguments);
	if (!lone_is_nil(lone_list_rest(arguments))) { /* too many arguments: (sign 1 2 3) */ linux_exit(-1); }

	if (lone_is_integer(value)) {
		return lone_integer_create(lone, value->integer > 0? 1 : value->integer < 0? -1 : 0);
	} else {
		linux_exit(-1);
	}
}

static struct lone_value *lone_primitive_is_zero(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *value = lone_primitive_sign(lone, module, environment, arguments, closure);
	if (lone_is_integer(value) && value->integer == 0) { return value; }
	else { return lone_nil(lone); }
}

static struct lone_value *lone_primitive_is_positive(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *value = lone_primitive_sign(lone, module, environment, arguments, closure);
	if (lone_is_integer(value) && value->integer > 0) { return value; }
	else { return lone_nil(lone); }
}

static struct lone_value *lone_primitive_is_negative(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *value = lone_primitive_sign(lone, module, environment, arguments, closure);
	if (lone_is_integer(value) && value->integer < 0) { return value; }
	else { return lone_nil(lone); }
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Text operations.                                                    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_primitive_join(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	return lone_text_transfer_bytes(lone, lone_join(lone, lone_list_first(arguments), lone_list_rest(arguments), lone_is_text), true);
}

static struct lone_value *lone_primitive_concatenate(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	return lone_text_transfer_bytes(lone, lone_concatenate(lone, arguments, lone_is_text), true);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    List operations.                                                    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_primitive_construct(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *first, *rest;

	if (lone_is_nil(arguments)) { /* no arguments given: (construct) */ linux_exit(-1); }

	first = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (lone_is_nil(arguments)) { /* only one argument given: (construct first) */ linux_exit(-1); }

	rest = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (!lone_is_nil(arguments)) { /* more than two arguments given: (construct first rest extra) */ linux_exit(-1); }

	return lone_list_create(lone, first, rest);
}

static struct lone_value *lone_primitive_first(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *argument;
	if (lone_is_nil(arguments)) { linux_exit(-1); }
	argument = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (lone_is_nil(argument)) { linux_exit(-1); }
	if (!lone_is_nil(arguments)) { linux_exit(-1); }
	return lone_list_first(argument);
}

static struct lone_value *lone_primitive_rest(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *argument;
	if (lone_is_nil(arguments)) { linux_exit(-1); }
	argument = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (lone_is_nil(argument)) { linux_exit(-1); }
	if (!lone_is_nil(arguments)) { linux_exit(-1); }
	return lone_list_rest(argument);
}

static struct lone_value *lone_primitive_list_map(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *function, *list, *results, *head;

	if (lone_is_nil(arguments)) { /* arguments not given */ linux_exit(-1); }
	function = lone_list_first(arguments);
	if (!lone_is_applicable(function)) { /* not given an applicable value */ linux_exit(-1); }
	arguments = lone_list_rest(arguments);
	list = lone_list_first(arguments);
	if (!lone_is_list(list)) { /* can only map functions to lists */ linux_exit(-1); }
	arguments = lone_list_rest(arguments);
	if (!lone_is_nil(arguments)) { /* too many arguments given */ linux_exit(-1); }

	results = lone_list_create_nil(lone);

	for (head = results; !lone_is_nil(list); list = lone_list_rest(list)) {
		arguments = lone_list_create(lone, lone_list_first(list), lone_nil(lone));
		head = lone_list_append(lone, head, lone_apply(lone, module, environment, function, arguments));
	}

	return results;
}

static struct lone_value *lone_primitive_list_reduce(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *function, *list, *result;

	if (lone_is_nil(arguments)) { /* arguments not given */ linux_exit(-1); }
	function = lone_list_first(arguments);
	if (!lone_is_applicable(function)) { /* not given an applicable value */ linux_exit(-1); }
	arguments = lone_list_rest(arguments);
	result = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	list = lone_list_first(arguments);
	if (!lone_is_list(list)) { /* can only map functions to lists */ linux_exit(-1); }
	arguments = lone_list_rest(arguments);
	if (!lone_is_nil(arguments)) { /* too many arguments given */ linux_exit(-1); }

	for (/* list */; !lone_is_nil(list); list = lone_list_rest(list)) {
		arguments = lone_list_build(lone, 2, result, lone_list_first(list));
		result = lone_apply(lone, module, environment, function, arguments);
	}

	return result;
}

static struct lone_value *lone_primitive_flatten(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	return lone_list_flatten(lone, arguments);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Linux primitive functions for issuing system calls.                 │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static inline long lone_value_to_linux_system_call_number(struct lone_lisp *lone, struct lone_value *linux_system_call_table, struct lone_value *value)
{
	switch (value->type) {
	case LONE_INTEGER:
		return value->integer;
	case LONE_BYTES:
	case LONE_TEXT:
	case LONE_SYMBOL:
		return lone_table_get(lone, linux_system_call_table, value)->integer;
	case LONE_MODULE:
	case LONE_FUNCTION:
	case LONE_PRIMITIVE:
	case LONE_LIST:
	case LONE_VECTOR:
	case LONE_TABLE:
	case LONE_POINTER:
		linux_exit(-1);
	}
}

static inline long lone_value_to_linux_system_call_argument(struct lone_value *value)
{
	switch (value->type) {
	case LONE_INTEGER: return value->integer;
	case LONE_POINTER: return (long) value->pointer.address;
	case LONE_BYTES: case LONE_TEXT: case LONE_SYMBOL: return (long) value->bytes.pointer;
	case LONE_PRIMITIVE: return (long) value->primitive.function;
	case LONE_FUNCTION: case LONE_LIST: case LONE_VECTOR: case LONE_TABLE: case LONE_MODULE: linux_exit(-1);
	}
}

static struct lone_value *lone_primitive_linux_system_call(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *linux_system_call_table)
{
	struct lone_value *argument;
	long result, number, args[6];
	unsigned char i;

	if (lone_is_nil(arguments)) { /* need at least the system call number */ linux_exit(-1); }
	argument = lone_list_first(arguments);
	number = lone_value_to_linux_system_call_number(lone, linux_system_call_table, argument);
	arguments = lone_list_rest(arguments);

	for (i = 0; i < 6; ++i) {
		if (lone_is_nil(arguments)) {
			args[i] = 0;
		} else {
			argument = lone_list_first(arguments);
			args[i] = lone_value_to_linux_system_call_argument(argument);
			arguments = lone_list_rest(arguments);
		}
	}

	if (!lone_is_nil(arguments)) { /* too many arguments given */ linux_exit(-1); }

	result = linux_system_call_6(number, args[0], args[1], args[2], args[3], args[4], args[5]);

	return lone_integer_create(lone, result);
}

/* ╭─────────────────────────┨ LONE LINUX PROCESS ┠─────────────────────────╮
   │                                                                        │
   │    Code to access all the parameters Linux passes to its processes.    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
struct auxiliary {
	long type;
	union {
		char *c_string;
		void *pointer;
		long integer;
	} as;
};

static struct lone_bytes lone_get_auxiliary_random_bytes(struct auxiliary *value)
{
	struct lone_bytes random = { 0, 0 };

	for (/* value */; value->type != AT_NULL; ++value) {
		if (value->type == AT_RANDOM) {
			random.pointer = value->as.pointer;
			random.count = 16;
		}
	}

	return random;
}

static void lone_auxiliary_value_to_table(struct lone_lisp *lone, struct lone_value *table, struct auxiliary *auxiliary_value)
{
	struct lone_value *key, *value;
	switch (auxiliary_value->type) {
	case AT_BASE_PLATFORM:
		key = lone_intern_c_string(lone, "base-platform");
		value = lone_text_create_from_c_string(lone, auxiliary_value->as.c_string);
		break;
	case AT_PLATFORM:
		key = lone_intern_c_string(lone, "platform");
		value = lone_text_create_from_c_string(lone, auxiliary_value->as.c_string);
		break;
	case AT_HWCAP:
		key = lone_intern_c_string(lone, "hardware-capabilities");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_HWCAP2:
		key = lone_intern_c_string(lone, "hardware-capabilities-2");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_FLAGS:
		key = lone_intern_c_string(lone, "flags");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_NOTELF:
		key = lone_intern_c_string(lone, "not-ELF");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_BASE:
		key = lone_intern_c_string(lone, "interpreter-base-address");
		value = lone_pointer_create(lone, auxiliary_value->as.pointer, LONE_TO_UNKNOWN);
		break;
	case AT_ENTRY:
		key = lone_intern_c_string(lone, "entry-point");
		value = lone_pointer_create(lone, auxiliary_value->as.pointer, LONE_TO_UNKNOWN);
		break;
	case AT_SYSINFO_EHDR:
		key = lone_intern_c_string(lone, "vDSO");
		value = lone_pointer_create(lone, auxiliary_value->as.pointer, LONE_TO_UNKNOWN);
		break;
	case AT_PHDR:
		key = lone_intern_c_string(lone, "program-headers-address");
		value = lone_pointer_create(lone, auxiliary_value->as.pointer, LONE_TO_UNKNOWN);
		break;
	case AT_PHENT:
		key = lone_intern_c_string(lone, "program-headers-entry-size");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_PHNUM:
		key = lone_intern_c_string(lone, "program-headers-count");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_EXECFN:
		key = lone_intern_c_string(lone, "executable-file-name");
		value = lone_text_create_from_c_string(lone, auxiliary_value->as.c_string);
		break;
	case AT_EXECFD:
		key = lone_intern_c_string(lone, "executable-file-descriptor");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_UID:
		key = lone_intern_c_string(lone, "user-id");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_EUID:
		key = lone_intern_c_string(lone, "effective-user-id");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_GID:
		key = lone_intern_c_string(lone, "group-id");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_EGID:
		key = lone_intern_c_string(lone, "effective-group-id");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_PAGESZ:
		key = lone_intern_c_string(lone, "page-size");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
#ifdef AT_MINSIGSTKSZ
	case AT_MINSIGSTKSZ:
		key = lone_intern_c_string(lone, "minimum-signal-delivery-stack-size");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
#endif
	case AT_CLKTCK:
		key = lone_intern_c_string(lone, "clock-tick");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_RANDOM:
		key = lone_intern_c_string(lone, "random");
		value = lone_bytes_create(lone, auxiliary_value->as.pointer, 16);
		break;
	case AT_SECURE:
		key = lone_intern_c_string(lone, "secure");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	default:
		key = lone_intern_c_string(lone, "unknown");
		value = lone_list_create(lone,
		                         lone_integer_create(lone, auxiliary_value->type),
		                         lone_integer_create(lone, auxiliary_value->as.integer));
	}

	lone_table_set(lone, table, key, value);
}

static struct lone_value *lone_auxiliary_vector_to_table(struct lone_lisp *lone, struct auxiliary *auxiliary_values)
{
	struct lone_value *table = lone_table_create(lone, 32, 0);
	size_t i;

	for (i = 0; auxiliary_values[i].type != AT_NULL; ++i) {
		lone_auxiliary_value_to_table(lone, table, &auxiliary_values[i]);
	}

	return table;
}

static struct lone_value *lone_environment_to_table(struct lone_lisp *lone, char **c_strings)
{
	struct lone_value *table = lone_table_create(lone, 64, 0), *key, *value;
	char *c_string_key, *c_string_value, *c_string;

	for (/* c_strings */; *c_strings; ++c_strings) {
		c_string = *c_strings;
		c_string_key = c_string;
		c_string_value = "";

		while (*c_string++) {
			if (*c_string == '=') {
				*c_string = '\0';
				c_string_value = c_string + 1;
				break;
			}
		}

		key = lone_text_create_from_c_string(lone, c_string_key);
		value = lone_text_create_from_c_string(lone, c_string_value);
		lone_table_set(lone, table, key, value);
	}

	return table;
}

static struct lone_value *lone_arguments_to_list(struct lone_lisp *lone, int count, char **c_strings)
{
	struct lone_value *arguments = lone_list_create_nil(lone), *head;
	int i;

	for (i = 0, head = arguments; i < count; ++i) {
		head = lone_list_append(lone, head, lone_text_create_from_c_string(lone, c_strings[i]));
	}

	return arguments;
}

static void lone_fill_linux_system_call_table(struct lone_lisp *lone, struct lone_value *linux_system_call_table)
{
	size_t i;

	static struct linux_system_call {
		char *symbol;
		int number;
	} linux_system_calls[] = {

		/* huge generated array initializer with all the system calls found on the host platform */
		#include <lone/NR.c>

	};

	for (i = 0; i < (sizeof(linux_system_calls)/sizeof(linux_system_calls[0])); ++i) {
		lone_table_set(lone, linux_system_call_table,
		               lone_intern_c_string(lone, linux_system_calls[i].symbol),
		               lone_integer_create(lone, linux_system_calls[i].number));
	}
}

/* ╭─────────────────────────┨ LONE LISP MODULES ┠──────────────────────────╮
   │                                                                        │
   │    Built-in modules containing essential functionality.                │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static void lone_builtin_module_linux_initialize(struct lone_lisp *lone, int argc, char **argv, char **envp, struct auxiliary *auxv)
{
	struct lone_value *name = lone_intern_c_string(lone, "linux"),
	                  *module = lone_module_for_name(lone, name),
	                  *linux_system_call_table = lone_table_create(lone, 1024, 0),
	                  *count, *arguments, *environment, *auxiliary_values,
	                  *primitive;

	struct lone_function_flags flags = { .evaluate_arguments = true, .evaluate_result = false, .variable_arguments = true };
	primitive = lone_primitive_create(lone, "linux_system_call", lone_primitive_linux_system_call, linux_system_call_table, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "system-call"), primitive);

	lone_set_and_export(lone, module, lone_intern_c_string(lone, "system-call-table"), linux_system_call_table);

	lone_fill_linux_system_call_table(lone, linux_system_call_table);

	count = lone_integer_create(lone, argc);
	arguments = lone_arguments_to_list(lone, argc, argv);
	environment = lone_environment_to_table(lone, envp);
	auxiliary_values = lone_auxiliary_vector_to_table(lone, auxv);

	lone_set_and_export(lone, module, lone_intern_c_string(lone, "argument-count"), count);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "arguments"), arguments);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "environment"), environment);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "auxiliary-values"), auxiliary_values);

	lone_table_set(lone, lone->modules.loaded, name, module);
}

static void lone_builtin_module_math_initialize(struct lone_lisp *lone)
{
	struct lone_value *name = lone_intern_c_string(lone, "math"),
	                  *module = lone_module_for_name(lone, name),
	                  *primitive;

	struct lone_function_flags flags = { .evaluate_arguments = true, .evaluate_result = false, .variable_arguments = true };

	primitive = lone_primitive_create(lone, "add", lone_primitive_add, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "+"), primitive);

	primitive = lone_primitive_create(lone, "subtract", lone_primitive_subtract, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "-"), primitive);

	primitive = lone_primitive_create(lone, "multiply", lone_primitive_multiply, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "*"), primitive);

	primitive = lone_primitive_create(lone, "divide", lone_primitive_divide, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "/"), primitive);

	primitive = lone_primitive_create(lone, "is_less_than", lone_primitive_is_less_than, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "<"), primitive);

	primitive = lone_primitive_create(lone, "is_less_than_or_equal_to", lone_primitive_is_less_than_or_equal_to, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "<="), primitive);

	primitive = lone_primitive_create(lone, "is_greater_than", lone_primitive_is_greater_than, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, ">"), primitive);

	primitive = lone_primitive_create(lone, "is_greater_than_or_equal_to", lone_primitive_is_greater_than_or_equal_to, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, ">="), primitive);

	primitive = lone_primitive_create(lone, "sign", lone_primitive_sign, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "sign"), primitive);

	primitive = lone_primitive_create(lone, "is_zero", lone_primitive_is_zero, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "zero?"), primitive);

	primitive = lone_primitive_create(lone, "is_positive", lone_primitive_is_positive, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "positive?"), primitive);

	primitive = lone_primitive_create(lone, "is_negative", lone_primitive_is_negative, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "negative?"), primitive);

	lone_table_set(lone, lone->modules.loaded, name, module);
}

static void lone_builtin_module_text_initialize(struct lone_lisp *lone)
{
	struct lone_value *name = lone_intern_c_string(lone, "text"),
	                  *module = lone_module_for_name(lone, name),
	                  *primitive;

	struct lone_function_flags flags = { .evaluate_arguments = true, .evaluate_result = false, .variable_arguments = true };

	primitive = lone_primitive_create(lone, "join", lone_primitive_join, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "join"), primitive);

	primitive = lone_primitive_create(lone, "concatenate", lone_primitive_concatenate, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "concatenate"), primitive);

	lone_table_set(lone, lone->modules.loaded, name, module);
}

static void lone_builtin_module_list_initialize(struct lone_lisp *lone)
{
	struct lone_value *name = lone_intern_c_string(lone, "list"),
	                  *module = lone_module_for_name(lone, name),
	                  *primitive;

	struct lone_function_flags flags = { .evaluate_arguments = true, .evaluate_result = false, .variable_arguments = true };

	primitive = lone_primitive_create(lone, "construct", lone_primitive_construct, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "construct"), primitive);

	primitive = lone_primitive_create(lone, "first", lone_primitive_first, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "first"), primitive);

	primitive = lone_primitive_create(lone, "rest", lone_primitive_rest, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "rest"), primitive);

	primitive = lone_primitive_create(lone, "map", lone_primitive_list_map, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "map"), primitive);

	primitive = lone_primitive_create(lone, "reduce", lone_primitive_list_reduce, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "reduce"), primitive);

	primitive = lone_primitive_create(lone, "flatten", lone_primitive_flatten, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "flatten"), primitive);

	lone_table_set(lone, lone->modules.loaded, name, module);
}

static void lone_modules_initialize(struct lone_lisp *lone, int argc, char **argv, char **envp, struct auxiliary *auxv)
{
	lone_builtin_module_linux_initialize(lone, argc, argv, envp, auxv);
	lone_module_lone_initialize(lone);
	lone_builtin_module_math_initialize(lone);
	lone_builtin_module_text_initialize(lone);
	lone_builtin_module_list_initialize(lone);

	lone_vector_push_all(lone, lone->modules.path, 4,

		lone_text_create_from_c_string(lone, "."),
		lone_text_create_from_c_string(lone, "~/.lone/modules"),
		lone_text_create_from_c_string(lone, "~/.local/lib/lone/modules"),
		lone_text_create_from_c_string(lone, "/usr/lib/lone/modules")

	);
}

/* ╭───────────────────────┨ LONE LISP ENTRY POINT ┠────────────────────────╮
   │                                                                        │
   │    Linux places argument, environment and auxiliary value arrays       │
   │    on the stack before jumping to the entry point of the process.      │
   │    Architecture-specific code collects this data and passes it to      │
   │    the lone function which begins execution of the lisp code.          │
   │                                                                        │
   │    During early initialization, lone has no dynamic memory             │
   │    allocation capabilities and so this function statically             │
   │    allocates 64 KiB of memory for the early bootstrapping process.     │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

#include <lone/architecture/linux/entry_point.c>

long lone(int argc, char **argv, char **envp, struct auxiliary *auxv)
{
	void *stack = __builtin_frame_address(0);
	static unsigned char __attribute__((aligned(LONE_ALIGNMENT))) bytes[LONE_MEMORY_SIZE];
	struct lone_bytes memory = { sizeof(bytes), bytes }, random = lone_get_auxiliary_random_bytes(auxv);
	struct lone_lisp lone;

	lone_lisp_initialize(&lone, memory, 1024, stack, random);
	lone_modules_initialize(&lone, argc, argv, envp, auxv);

	lone_module_load_null_from_standard_input(&lone);

	return 0;
}
