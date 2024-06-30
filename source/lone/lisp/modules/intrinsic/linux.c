/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/modules/intrinsic/linux.h>

#include <lone/lisp/module.h>

#include <lone/lisp/value/primitive.h>
#include <lone/lisp/value/list.h>
#include <lone/lisp/value/vector.h>
#include <lone/lisp/value/table.h>
#include <lone/lisp/value/bytes.h>
#include <lone/lisp/value/text.h>
#include <lone/lisp/value/symbol.h>
#include <lone/lisp/value/integer.h>
#include <lone/lisp/value/pointer.h>

#include <lone/linux.h>

static void lone_lisp_auxiliary_value_to_table(struct lone_lisp *lone,
		struct lone_lisp_value table, struct lone_lisp_value unknowns,
		struct lone_auxiliary_vector *auxiliary)
{
	struct lone_lisp_value key, value;

	switch (auxiliary->type) {

#ifdef AT_BASE_PLATFORM
	case AT_BASE_PLATFORM:
		key = lone_lisp_intern_c_string(lone, "base-platform");
		value = lone_lisp_text_from_c_string(lone, auxiliary->value.as.c_string);
		break;
#endif

#ifdef AT_PLATFORM
	case AT_PLATFORM:
		key = lone_lisp_intern_c_string(lone, "platform");
		value = lone_lisp_text_from_c_string(lone, auxiliary->value.as.c_string);
		break;
#endif

#ifdef AT_HWCAP
	case AT_HWCAP:
		key = lone_lisp_intern_c_string(lone, "hardware-capabilities");
		value = lone_lisp_integer_create(auxiliary->value.as.signed_integer);
		break;
#endif

#ifdef AT_HWCAP2
	case AT_HWCAP2:
		key = lone_lisp_intern_c_string(lone, "hardware-capabilities-2");
		value = lone_lisp_integer_create(auxiliary->value.as.signed_integer);
		break;
#endif

#ifdef AT_FLAGS
	case AT_FLAGS:
		key = lone_lisp_intern_c_string(lone, "flags");
		value = lone_lisp_integer_create(auxiliary->value.as.signed_integer);
		break;
#endif

#ifdef AT_NOTELF
	case AT_NOTELF:
		key = lone_lisp_intern_c_string(lone, "not-ELF");
		value = lone_lisp_integer_create(auxiliary->value.as.signed_integer);
		break;
#endif

#ifdef AT_BASE
	case AT_BASE:
		key = lone_lisp_intern_c_string(lone, "interpreter-base-address");
		value = lone_lisp_pointer_create(auxiliary->value.as.pointer, LONE_TO_UNKNOWN);
		break;
#endif

#ifdef AT_ENTRY
	case AT_ENTRY:
		key = lone_lisp_intern_c_string(lone, "entry-point");
		value = lone_lisp_pointer_create(auxiliary->value.as.pointer, LONE_TO_UNKNOWN);
		break;
#endif

#ifdef AT_SYSINFO_EHDR
	case AT_SYSINFO_EHDR:
		key = lone_lisp_intern_c_string(lone, "vDSO");
		value = lone_lisp_pointer_create(auxiliary->value.as.pointer, LONE_TO_UNKNOWN);
		break;
#endif

#ifdef AT_PHDR
	case AT_PHDR:
		key = lone_lisp_intern_c_string(lone, "program-header-table-address");
		value = lone_lisp_pointer_create(auxiliary->value.as.pointer, LONE_TO_UNKNOWN);
		break;
#endif

#ifdef AT_PHENT
	case AT_PHENT:
		key = lone_lisp_intern_c_string(lone, "program-header-table-entry-size");
		value = lone_lisp_integer_create(auxiliary->value.as.signed_integer);
		break;
#endif

#ifdef AT_PHNUM
	case AT_PHNUM:
		key = lone_lisp_intern_c_string(lone, "program-header-table-entry-count");
		value = lone_lisp_integer_create(auxiliary->value.as.signed_integer);
		break;
#endif

#ifdef AT_EXECFN
	case AT_EXECFN:
		key = lone_lisp_intern_c_string(lone, "executable-file-name");
		value = lone_lisp_text_from_c_string(lone, auxiliary->value.as.c_string);
		break;
#endif

#ifdef AT_EXECFD
	case AT_EXECFD:
		key = lone_lisp_intern_c_string(lone, "executable-file-descriptor");
		value = lone_lisp_integer_create(auxiliary->value.as.signed_integer);
		break;
#endif

#ifdef AT_UID
	case AT_UID:
		key = lone_lisp_intern_c_string(lone, "user-id");
		value = lone_lisp_integer_create(auxiliary->value.as.signed_integer);
		break;
#endif

#ifdef AT_EUID
	case AT_EUID:
		key = lone_lisp_intern_c_string(lone, "effective-user-id");
		value = lone_lisp_integer_create(auxiliary->value.as.signed_integer);
		break;
#endif

#ifdef AT_GID
	case AT_GID:
		key = lone_lisp_intern_c_string(lone, "group-id");
		value = lone_lisp_integer_create(auxiliary->value.as.signed_integer);
		break;
#endif

#ifdef AT_EGID
	case AT_EGID:
		key = lone_lisp_intern_c_string(lone, "effective-group-id");
		value = lone_lisp_integer_create(auxiliary->value.as.signed_integer);
		break;
#endif

#ifdef AT_PAGESZ
	case AT_PAGESZ:
		key = lone_lisp_intern_c_string(lone, "page-size");
		value = lone_lisp_integer_create(auxiliary->value.as.signed_integer);
		break;
#endif

#ifdef AT_CLKTCK
	case AT_CLKTCK:
		key = lone_lisp_intern_c_string(lone, "clock-tick");
		value = lone_lisp_integer_create(auxiliary->value.as.signed_integer);
		break;
#endif

#ifdef AT_RANDOM
	case AT_RANDOM:
		key = lone_lisp_intern_c_string(lone, "random");
		value = lone_lisp_bytes_transfer(lone, auxiliary->value.as.bytes, 16, false);
		break;
#endif

#ifdef AT_SECURE
	case AT_SECURE:
		key = lone_lisp_intern_c_string(lone, "secure");
		value = lone_lisp_integer_create(auxiliary->value.as.signed_integer);
		break;
#endif

#ifdef AT_MINSIGSTKSZ
	case AT_MINSIGSTKSZ:
		key = lone_lisp_intern_c_string(lone, "minimum-signal-delivery-stack-size");
		value = lone_lisp_integer_create(auxiliary->value.as.signed_integer);
		break;
#endif

#ifdef AT_RSEQ_FEATURE_SIZE
	case AT_RSEQ_FEATURE_SIZE:
		key = lone_lisp_intern_c_string(lone, "restartable-sequences-supported-feature-size");
		value = lone_lisp_integer_create(auxiliary->value.as.signed_integer);
		break;
#endif

#ifdef AT_RSEQ_ALIGN
	case AT_RSEQ_ALIGN:
		key = lone_lisp_intern_c_string(lone, "restartable-sequences-allocation-alignment");
		value = lone_lisp_integer_create(auxiliary->value.as.signed_integer);
		break;
#endif

	default:
		key = lone_lisp_integer_create((lone_lisp_integer) auxiliary->type);
		value = lone_lisp_integer_create(auxiliary->value.as.signed_integer);
		lone_lisp_table_set(lone, unknowns, key, value);
		return;
	}

	lone_lisp_table_set(lone, table, key, value);
}

static struct lone_lisp_value lone_lisp_auxiliary_vector_to_table(struct lone_lisp *lone, struct lone_auxiliary_vector *auxiliary_vector)
{
	struct lone_lisp_value table = lone_lisp_table_create(lone, 32, lone_lisp_nil());
	struct lone_lisp_value unknowns = lone_lisp_table_create(lone, 2, lone_lisp_nil());
	size_t i;

	for (i = 0; auxiliary_vector[i].type != AT_NULL; ++i) {
		lone_lisp_auxiliary_value_to_table(lone, table, unknowns, &auxiliary_vector[i]);
	}

	if (unknowns.as.heap_value->as.table.count) {
		lone_lisp_table_set(lone, table, lone_lisp_intern_c_string(lone, "unknown"), unknowns);
	}

	return table;
}

static struct lone_lisp_value lone_lisp_environment_to_table(struct lone_lisp *lone, char **c_strings)
{
	struct lone_lisp_value table = lone_lisp_table_create(lone, 64, lone_lisp_nil()), key, value;
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

		key = lone_lisp_text_from_c_string(lone, c_string_key);
		value = lone_lisp_text_from_c_string(lone, c_string_value);
		lone_lisp_table_set(lone, table, key, value);
	}

	return table;
}

static struct lone_lisp_value lone_lisp_arguments_to_vector(struct lone_lisp *lone, int count, char **c_strings)
{
	struct lone_lisp_value arguments;
	int i;

	arguments = lone_lisp_vector_create(lone, (size_t) count);

	for (i = 0; i < count; ++i) {
		lone_lisp_vector_set(lone,
		                arguments,
		                lone_lisp_integer_create(i),
		                lone_lisp_text_from_c_string(lone, c_strings[i]));
	}

	return arguments;
}

static void lone_lisp_fill_linux_system_call_table(struct lone_lisp *lone, struct lone_lisp_value linux_system_call_table)
{
	size_t i;

	static struct linux_system_call {
		char *symbol;
		lone_lisp_integer number;
	} linux_system_calls[] = {

		/* huge generated array initializer with all the system calls found on the host platform */
		#include <lone/lisp/modules/intrinsic/linux/NR.c>

	};

	for (i = 0; i < (sizeof(linux_system_calls)/sizeof(linux_system_calls[0])); ++i) {
		lone_lisp_table_set(lone, linux_system_call_table,
				lone_lisp_intern_c_string(lone, linux_system_calls[i].symbol),
				lone_lisp_integer_create(linux_system_calls[i].number));
	}
}

void lone_lisp_modules_intrinsic_linux_initialize(struct lone_lisp *lone,
		int argc, char **argv, char **envp,
		struct lone_auxiliary_vector *auxv)
{
	struct lone_lisp_value name, module, linux_system_call_table, count, arguments, environment, auxiliary_vector, primitive;
	struct lone_lisp_function_flags flags;

	name = lone_lisp_intern_c_string(lone, "linux");
	module = lone_lisp_module_for_name(lone, name);
	linux_system_call_table = lone_lisp_table_create(lone, 1024, lone_lisp_nil());
	flags = (struct lone_lisp_function_flags) { .evaluate_arguments = true, .evaluate_result = false };

	primitive = lone_lisp_primitive_create(lone, "linux_system_call", lone_lisp_primitive_linux_system_call, linux_system_call_table, flags);

	lone_lisp_fill_linux_system_call_table(lone, linux_system_call_table);

	count = lone_lisp_integer_create(argc);
	arguments = lone_lisp_arguments_to_vector(lone, argc, argv);
	environment = lone_lisp_environment_to_table(lone, envp);
	auxiliary_vector = lone_lisp_auxiliary_vector_to_table(lone, auxv);

	lone_lisp_module_set_and_export_c_string(lone, module, "argument-count", count);
	lone_lisp_module_set_and_export_c_string(lone, module, "arguments", arguments);
	lone_lisp_module_set_and_export_c_string(lone, module, "environment", environment);
	lone_lisp_module_set_and_export_c_string(lone, module, "auxiliary-vector", auxiliary_vector);

	lone_lisp_module_set_and_export_c_string(lone, module, "system-call-table", linux_system_call_table);
	lone_lisp_module_set_and_export_c_string(lone, module, "system-call", primitive);

	lone_lisp_table_set(lone, lone->modules.loaded, name, module);
}

static inline long lone_lisp_value_to_linux_system_call_number(struct lone_lisp *lone,
		struct lone_lisp_value linux_system_call_table, struct lone_lisp_value value)
{
	struct lone_lisp_heap_value *actual;

	switch (value.type) {
	case LONE_LISP_TYPE_INTEGER:
		return value.as.integer;
	case LONE_LISP_TYPE_NIL:
	case LONE_LISP_TYPE_POINTER:
		linux_exit(-1);
	case LONE_LISP_TYPE_HEAP_VALUE:
		break;
	}

	actual = value.as.heap_value;

	switch (actual->type) {
	case LONE_LISP_TYPE_TEXT:
		value = lone_lisp_text_to_symbol(lone, value);
		__attribute__((fallthrough));
	case LONE_LISP_TYPE_SYMBOL:
		return lone_lisp_table_get(lone, linux_system_call_table, value).as.integer;
	case LONE_LISP_TYPE_BYTES:
	case LONE_LISP_TYPE_MODULE:
	case LONE_LISP_TYPE_FUNCTION:
	case LONE_LISP_TYPE_PRIMITIVE:
	case LONE_LISP_TYPE_LIST:
	case LONE_LISP_TYPE_VECTOR:
	case LONE_LISP_TYPE_TABLE:
		linux_exit(-1);
	}
}

static inline long lone_lisp_value_to_linux_system_call_argument(struct lone_lisp_value value)
{
	struct lone_lisp_heap_value *actual;

	switch (value.type) {
	case LONE_LISP_TYPE_NIL:
		return 0;
	case LONE_LISP_TYPE_POINTER:
		return (long) value.as.pointer.to_void;
	case LONE_LISP_TYPE_INTEGER:
		return (long) value.as.integer;
	case LONE_LISP_TYPE_HEAP_VALUE:
		break;
	}

	actual = value.as.heap_value;

	switch (actual->type) {
	case LONE_LISP_TYPE_BYTES:
	case LONE_LISP_TYPE_TEXT:
	case LONE_LISP_TYPE_SYMBOL:
		return (long) actual->as.bytes.pointer;
	case LONE_LISP_TYPE_PRIMITIVE:
		return (long) actual->as.primitive.function;
	case LONE_LISP_TYPE_FUNCTION:
	case LONE_LISP_TYPE_LIST:
	case LONE_LISP_TYPE_VECTOR:
	case LONE_LISP_TYPE_TABLE:
	case LONE_LISP_TYPE_MODULE:
		linux_exit(-1);
	}
}

LONE_LISP_PRIMITIVE(linux_system_call)
{
	struct lone_lisp_value linux_system_call_table, argument;
	long result, number, args[6];
	unsigned char i;

	linux_system_call_table = closure;

	if (lone_lisp_is_nil(arguments)) { /* need at least the system call number */ linux_exit(-1); }
	argument = lone_lisp_list_first(arguments);
	number = lone_lisp_value_to_linux_system_call_number(lone, linux_system_call_table, argument);
	arguments = lone_lisp_list_rest(arguments);

	for (i = 0; i < 6; ++i) {
		if (lone_lisp_is_nil(arguments)) {
			args[i] = 0;
		} else {
			argument = lone_lisp_list_first(arguments);
			args[i] = lone_lisp_value_to_linux_system_call_argument(argument);
			arguments = lone_lisp_list_rest(arguments);
		}
	}

	if (!lone_lisp_is_nil(arguments)) { /* too many arguments given */ linux_exit(-1); }

	result = linux_system_call_6(number, args[0], args[1], args[2], args[3], args[4], args[5]);

	return lone_lisp_integer_create(result);
}
