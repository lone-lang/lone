/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/definitions.h>
#include <lone/types.h>

#include <lone/modules.h>
#include <lone/modules/intrinsic/linux.h>

#include <lone/value/primitive.h>
#include <lone/value/list.h>
#include <lone/value/table.h>
#include <lone/value/bytes.h>
#include <lone/value/text.h>
#include <lone/value/symbol.h>
#include <lone/value/integer.h>
#include <lone/value/pointer.h>

#include <lone/linux.h>

static void lone_auxiliary_value_to_table(struct lone_lisp *lone, struct lone_value *table, struct lone_auxiliary_vector *auxiliary)
{
	struct lone_value *key, *value;

	switch (auxiliary->type) {
	case AT_BASE_PLATFORM:
		key = lone_intern_c_string(lone, "base-platform");
		value = lone_text_create_from_c_string(lone, auxiliary->value.as.c_string);
		break;
	case AT_PLATFORM:
		key = lone_intern_c_string(lone, "platform");
		value = lone_text_create_from_c_string(lone, auxiliary->value.as.c_string);
		break;
	case AT_HWCAP:
		key = lone_intern_c_string(lone, "hardware-capabilities");
		value = lone_integer_create(lone, auxiliary->value.as.integer);
		break;
	case AT_HWCAP2:
		key = lone_intern_c_string(lone, "hardware-capabilities-2");
		value = lone_integer_create(lone, auxiliary->value.as.integer);
		break;
	case AT_FLAGS:
		key = lone_intern_c_string(lone, "flags");
		value = lone_integer_create(lone, auxiliary->value.as.integer);
		break;
	case AT_NOTELF:
		key = lone_intern_c_string(lone, "not-ELF");
		value = lone_integer_create(lone, auxiliary->value.as.integer);
		break;
	case AT_BASE:
		key = lone_intern_c_string(lone, "interpreter-base-address");
		value = lone_pointer_create(lone, auxiliary->value.as.pointer, LONE_TO_UNKNOWN);
		break;
	case AT_ENTRY:
		key = lone_intern_c_string(lone, "entry-point");
		value = lone_pointer_create(lone, auxiliary->value.as.pointer, LONE_TO_UNKNOWN);
		break;
	case AT_SYSINFO_EHDR:
		key = lone_intern_c_string(lone, "vDSO");
		value = lone_pointer_create(lone, auxiliary->value.as.pointer, LONE_TO_UNKNOWN);
		break;
	case AT_PHDR:
		key = lone_intern_c_string(lone, "program-header-table-address");
		value = lone_pointer_create(lone, auxiliary->value.as.pointer, LONE_TO_UNKNOWN);
		break;
	case AT_PHENT:
		key = lone_intern_c_string(lone, "program-header-table-entry-size");
		value = lone_integer_create(lone, auxiliary->value.as.integer);
		break;
	case AT_PHNUM:
		key = lone_intern_c_string(lone, "program-header-table-entry-count");
		value = lone_integer_create(lone, auxiliary->value.as.integer);
		break;
	case AT_EXECFN:
		key = lone_intern_c_string(lone, "executable-file-name");
		value = lone_text_create_from_c_string(lone, auxiliary->value.as.c_string);
		break;
	case AT_EXECFD:
		key = lone_intern_c_string(lone, "executable-file-descriptor");
		value = lone_integer_create(lone, auxiliary->value.as.integer);
		break;
	case AT_UID:
		key = lone_intern_c_string(lone, "user-id");
		value = lone_integer_create(lone, auxiliary->value.as.integer);
		break;
	case AT_EUID:
		key = lone_intern_c_string(lone, "effective-user-id");
		value = lone_integer_create(lone, auxiliary->value.as.integer);
		break;
	case AT_GID:
		key = lone_intern_c_string(lone, "group-id");
		value = lone_integer_create(lone, auxiliary->value.as.integer);
		break;
	case AT_EGID:
		key = lone_intern_c_string(lone, "effective-group-id");
		value = lone_integer_create(lone, auxiliary->value.as.integer);
		break;
	case AT_PAGESZ:
		key = lone_intern_c_string(lone, "page-size");
		value = lone_integer_create(lone, auxiliary->value.as.integer);
		break;
#ifdef AT_MINSIGSTKSZ
	case AT_MINSIGSTKSZ:
		key = lone_intern_c_string(lone, "minimum-signal-delivery-stack-size");
		value = lone_integer_create(lone, auxiliary->value.as.integer);
		break;
#endif
	case AT_CLKTCK:
		key = lone_intern_c_string(lone, "clock-tick");
		value = lone_integer_create(lone, auxiliary->value.as.integer);
		break;
	case AT_RANDOM:
		key = lone_intern_c_string(lone, "random");
		value = lone_bytes_create(lone, auxiliary->value.as.pointer, 16);
		break;
	case AT_SECURE:
		key = lone_intern_c_string(lone, "secure");
		value = lone_integer_create(lone, auxiliary->value.as.integer);
		break;
	default:
		key = lone_intern_c_string(lone, "unknown");
		value = lone_list_create(lone,
		                         lone_integer_create(lone, auxiliary->type),
		                         lone_integer_create(lone, auxiliary->value.as.integer));
	}

	lone_table_set(lone, table, key, value);
}

static struct lone_value *lone_auxiliary_vector_to_table(struct lone_lisp *lone, struct lone_auxiliary_vector *auxiliary_vector)
{
	struct lone_value *table = lone_table_create(lone, 32, 0);
	size_t i;

	for (i = 0; auxiliary_vector[i].type != AT_NULL; ++i) {
		lone_auxiliary_value_to_table(lone, table, &auxiliary_vector[i]);
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

void lone_module_linux_initialize(struct lone_lisp *lone, int argc, char **argv, char **envp, struct lone_auxiliary_vector *auxv)
{
	struct lone_value *name = lone_intern_c_string(lone, "linux"),
	                  *module = lone_module_for_name(lone, name),
	                  *linux_system_call_table = lone_table_create(lone, 1024, 0),
	                  *count, *arguments, *environment, *auxiliary_vector,
	                  *primitive;

	struct lone_function_flags flags = { .evaluate_arguments = true, .evaluate_result = false, .variable_arguments = true };
	primitive = lone_primitive_create(lone, "linux_system_call", lone_primitive_linux_system_call, linux_system_call_table, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "system-call"), primitive);

	lone_set_and_export(lone, module, lone_intern_c_string(lone, "system-call-table"), linux_system_call_table);

	lone_fill_linux_system_call_table(lone, linux_system_call_table);

	count = lone_integer_create(lone, argc);
	arguments = lone_arguments_to_list(lone, argc, argv);
	environment = lone_environment_to_table(lone, envp);
	auxiliary_vector = lone_auxiliary_vector_to_table(lone, auxv);

	lone_set_and_export(lone, module, lone_intern_c_string(lone, "argument-count"), count);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "arguments"), arguments);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "environment"), environment);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "auxiliary-vector"), auxiliary_vector);

	lone_table_set(lone, lone->modules.loaded, name, module);
}

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

LONE_PRIMITIVE(linux_system_call)
{
	struct lone_value *linux_system_call_table = closure, *argument;
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
