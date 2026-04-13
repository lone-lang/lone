/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/module.h>

#include <lone/lisp/reader.h>
#include <lone/lisp/machine.h>
#include <lone/lisp/machine/stack.h>

#include <lone/lisp/garbage_collector.h>
#include <lone/lisp/utilities.h>

#include <lone/memory/allocator.h>

#include <lone/linux.h>

static bool lone_lisp_module_name_is_valid_component(struct lone_lisp *lone, struct lone_lisp_value symbol)
{
	struct lone_bytes name;
	size_t i;

	name = lone_lisp_symbol_name(lone, &symbol);

	if (name.count == 0) { return false; }

	/* reject path traversal components */
	if (name.count == 1) {
		if (name.pointer[0] == '.') {
			return false;
		}
	} else if (name.count == 2) {
		if (name.pointer[0] == '.' && name.pointer[1] == '.') {
			return false;
		}
	}

	/* reject components containing path separators and '\0' */
	for (i = 0; i < name.count; ++i) {
		if (name.pointer[i] == '/' || name.pointer[i] == 0) {
			return false;
		}
	}

	return true;
}

struct lone_lisp_value lone_lisp_module_null(struct lone_lisp *lone)
{
	return lone->modules.null;
}

static struct lone_lisp_value lone_lisp_module_name_to_key(struct lone_lisp *lone,
		struct lone_lisp_value name)
{
	struct lone_lisp_value head, component;

	switch (lone_lisp_type_of(name)) {
	case LONE_LISP_TAG_NIL:
	case LONE_LISP_TAG_FALSE:
	case LONE_LISP_TAG_TRUE:
	case LONE_LISP_TAG_INTEGER:
		/* invalid module name component */ linux_exit(-1);
	case LONE_LISP_TAG_SYMBOL:
		if (!lone_lisp_module_name_is_valid_component(lone, name)) {
			/* invalid module name component */ linux_exit(-1);
		}
		return lone_lisp_list_create(lone, name, lone_lisp_nil());
	case LONE_LISP_TAG_LIST:
		for (head = name; !lone_lisp_is_nil(head); head = lone_lisp_list_rest(lone, head)) {
			component = lone_lisp_list_first(lone, head);
			if (!lone_lisp_is_symbol(lone, component)) {
				/* not a symbol */ linux_exit(-1);
			}
			if (!lone_lisp_module_name_is_valid_component(lone, component)) {
				/* invalid module name component */ linux_exit(-1);
			}
		}
		return name;
	case LONE_LISP_TAG_MODULE:
		return lone_lisp_module_name_to_key(lone, lone_lisp_heap_value_of(lone, name)->as.module.name);
	case LONE_LISP_TAG_FUNCTION:
	case LONE_LISP_TAG_PRIMITIVE:
	case LONE_LISP_TAG_CONTINUATION:
	case LONE_LISP_TAG_GENERATOR:
	case LONE_LISP_TAG_TEXT:
	case LONE_LISP_TAG_BYTES:
	case LONE_LISP_TAG_VECTOR:
	case LONE_LISP_TAG_TABLE:
	default:
		/* invalid module name component */ linux_exit(-1);
	}
}

static bool lone_lisp_module_try_to_load_embedded(struct lone_lisp *lone,
		struct lone_lisp_value module, struct lone_lisp_value name)
{
	struct lone_lisp_value embedded_module;

	if (lone_lisp_is_nil(lone->modules.embedded)) { /* no embedded modules */ return false; }

	embedded_module = lone_lisp_table_get(lone, lone->modules.embedded, name);
	if (lone_lisp_is_nil(embedded_module)) { /* embedded module not found */ return false; }
	if (!lone_lisp_has_bytes(lone, embedded_module)) { /* invalid embedded module */ linux_exit(-1); }

	lone_lisp_module_load_from_bytes(lone, module, lone_lisp_heap_value_of(lone, embedded_module)->as.bytes);
	lone_lisp_table_delete(lone, lone->modules.embedded, name);
	return true;
}

static struct lone_lisp_value lone_lisp_module_get_or_create(struct lone_lisp *lone,
		struct lone_lisp_value name, bool *not_found)
{
	struct lone_lisp_value module;
	bool loaded;

	name = lone_lisp_module_name_to_key(lone, name);
	module = lone_lisp_table_get(lone, lone->modules.loaded, name);
	if (not_found) {
		*not_found = false;
	}

	if (lone_lisp_is_nil(module)) {
		module = lone_lisp_module_create(lone, name);
		lone_lisp_table_set(lone, lone->modules.loaded, name, module);

		loaded = lone_lisp_module_try_to_load_embedded(lone, module, name);

		if (not_found) {
			*not_found = !loaded;
		}
	}

	return module;
}

struct lone_lisp_value lone_lisp_module_for_name(struct lone_lisp *lone, struct lone_lisp_value name)
{
	return lone_lisp_module_get_or_create(lone, name, 0);
}

static int lone_lisp_module_search(struct lone_lisp *lone, struct lone_lisp_value symbols)
{
	struct lone_lisp_value arguments, package, search_path;
	struct lone_lisp_value slash, ln;
	struct lone_bytes path;
	long result;
	size_t i;

	symbols = lone_lisp_module_name_to_key(lone, symbols);
	package = lone_lisp_list_first(lone, symbols);

	slash = lone_lisp_intern_c_string(lone, "/");
	ln = lone_lisp_intern_c_string(lone, ".ln");

	LONE_LISP_VECTOR_FOR_EACH(lone, search_path, lone->modules.path, i) {
		arguments = lone_lisp_list_build(lone, 3, &search_path, &package, &symbols);
		arguments = lone_lisp_list_flatten(lone, arguments);
		arguments = lone_lisp_text_transfer_bytes(lone, lone_lisp_join(lone, slash, arguments, lone_lisp_has_bytes), true);
		arguments = lone_lisp_list_build(lone, 2, &arguments, &ln);
		path = lone_lisp_concatenate(lone, arguments, lone_lisp_has_bytes);

		result = linux_openat(AT_FDCWD, path.pointer, O_RDONLY | O_CLOEXEC);

		lone_memory_deallocate(lone->system, path.pointer, path.count + 1, 1, 1);

		if (result < 0) {
			switch (result) {
			case -ENOMEM: case -EFAULT:
				linux_exit(-1);
			default:
				continue;
			}
		}

		return (int) result;
	}

	linux_exit(-1); /* module not found */
}

static void lone_lisp_module_load_from_reader(struct lone_lisp *lone,
		struct lone_lisp_value module, struct lone_lisp_reader *reader)
{
	struct lone_lisp_machine machine;
	struct lone_lisp_value value;

	lone_lisp_machine_initialize(&machine,
		lone_lisp_machine_allocate_stack(lone, LONE_LISP_MACHINE_STACK_INITIAL_SIZE),
		LONE_LISP_MACHINE_STACK_INITIAL_SIZE);

	while (1) {
		value = lone_lisp_read(lone, reader);
		if (reader->status.error) { linux_exit(-1); }
		if (reader->status.end_of_input) { break; }

		lone_lisp_machine_reset(lone, &machine, module, value);
		while (lone_lisp_machine_cycle(lone, &machine));
		lone_lisp_garbage_collector(lone, &machine);
	}

	lone_lisp_reader_finalize(lone, reader);
	lone_lisp_garbage_collector(lone, &machine);
	lone_lisp_machine_deallocate_stack(lone, machine.stack);
}

void lone_lisp_module_load_from_bytes(struct lone_lisp *lone,
		struct lone_lisp_value module, struct lone_bytes bytes)
{
	struct lone_lisp_reader reader;

	lone_lisp_reader_for_bytes(lone, &reader, bytes);
	lone_lisp_module_load_from_reader(lone, module, &reader);
}

static void lone_lisp_module_load_from_file_descriptor(struct lone_lisp *lone,
		struct lone_lisp_value module, int file_descriptor)
{
	struct lone_lisp_reader reader;

	lone_lisp_reader_for_file_descriptor(lone, &reader, LONE_LISP_BUFFER_SIZE, file_descriptor);
	lone_lisp_module_load_from_reader(lone, module, &reader);
}

struct lone_lisp_value lone_lisp_module_load(struct lone_lisp *lone, struct lone_lisp_value name)
{
	struct lone_lisp_value module;
	bool not_found;
	int file_descriptor;

	module = lone_lisp_module_get_or_create(lone, name, &not_found);

	if (not_found) {
		file_descriptor = lone_lisp_module_search(lone, name);
		lone_lisp_module_load_from_file_descriptor(lone, module, file_descriptor);
		linux_close(file_descriptor);
	}

	return module;
}

void lone_lisp_module_load_null_from_file_descriptor(struct lone_lisp *lone, int file_descriptor)
{
	lone_lisp_module_load_from_file_descriptor(lone, lone_lisp_module_null(lone), file_descriptor);
}

void lone_lisp_module_load_null_from_standard_input(struct lone_lisp *lone)
{
	lone_lisp_module_load_null_from_file_descriptor(lone, 0);
}

void lone_lisp_module_export(struct lone_lisp *lone,
		struct lone_lisp_value module, struct lone_lisp_value symbol)
{
	if (!lone_lisp_is_symbol(lone, symbol)) { /* only symbols can be exported */ linux_exit(-1); }
	lone_lisp_vector_push(lone, lone_lisp_heap_value_of(lone, module)->as.module.exports, symbol);
}

void lone_lisp_module_set_and_export(struct lone_lisp *lone, struct lone_lisp_value module,
		struct lone_lisp_value symbol, struct lone_lisp_value value)
{
	lone_lisp_module_export(lone, module, symbol);
	lone_lisp_table_set(lone, lone_lisp_heap_value_of(lone, module)->as.module.environment, symbol, value);
}

void lone_lisp_module_set_and_export_c_string(struct lone_lisp *lone,
		struct lone_lisp_value module, char *symbol, struct lone_lisp_value value)
{
	lone_lisp_module_set_and_export(lone, module, lone_lisp_intern_c_string(lone, symbol), value);
}

void lone_lisp_module_export_primitive(struct lone_lisp *lone,
		struct lone_lisp_value module, char *symbol, char *name,
		lone_lisp_primitive_function function, struct lone_lisp_value closure,
		struct lone_lisp_function_flags flags)
{
	struct lone_lisp_value primitive;

	primitive = lone_lisp_primitive_create(lone, name, function, closure, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, symbol, primitive);
}

LONE_LISP_PRIMITIVE(module_export)
{
	struct lone_lisp_value arguments, head, symbol;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	for (head = arguments; !lone_lisp_is_nil(head); head = lone_lisp_list_rest(lone, head)) {
		symbol = lone_lisp_list_first(lone, head);

		lone_lisp_module_export(lone, machine->module, symbol);
	}

	lone_lisp_machine_push_value(lone, machine, lone_lisp_nil());
	return 0;
}

struct lone_lisp_import_specification {
	struct lone_lisp_value module;         /* module value to import from */
	struct lone_lisp_value symbols;        /* list of symbols to import */
	struct lone_lisp_value environment;    /* environment to import symbols to */

	bool prefixed;                     /* whether to prefix symbols */
};

static struct lone_lisp_value lone_prefix_module_name(struct lone_lisp *lone,
		struct lone_lisp_value module, struct lone_lisp_value symbol)
{
	struct lone_lisp_value arguments, separator;

	arguments = lone_lisp_list_flatten(lone, lone_lisp_list_build(lone, 2,
				&lone_lisp_heap_value_of(lone, module)->as.module.name, &symbol));
	separator = lone_lisp_intern_c_string(lone, ".");

	return lone_lisp_intern_bytes(lone, lone_lisp_join(lone, separator, arguments, lone_lisp_has_bytes), true);
}

static void lone_lisp_import_specification(struct lone_lisp *lone, struct lone_lisp_import_specification *spec)
{
	struct lone_lisp_value module, environment, exports, symbols, symbol, value;
	size_t i;

	module = spec->module;
	symbols = spec->symbols;
	environment = spec->environment;
	exports = lone_lisp_heap_value_of(lone, module)->as.module.exports;

	/* bind either the exported or the specified symbols: (import (module)), (import (module x f)) */
	LONE_LISP_VECTOR_FOR_EACH(lone, symbol, symbols, i) {
		if (!lone_lisp_is_symbol(lone, symbol)) { /* name not a symbol: (import (module 10)) */ linux_exit(-1); }

		if (!lone_lisp_vector_contains(lone, exports, symbol)) {
			/* attempt to import private symbol */ linux_exit(-1);
		}

		value = lone_lisp_table_get(lone, lone_lisp_heap_value_of(lone, module)->as.module.environment, symbol);

		if (spec->prefixed) {
			symbol = lone_prefix_module_name(lone, spec->module, symbol);
		}

		lone_lisp_table_set(lone, environment, symbol, value);
	}
}

static void lone_lisp_primitive_import_form(struct lone_lisp *lone,
		struct lone_lisp_import_specification *spec, struct lone_lisp_value argument)
{
	struct lone_lisp_value name;

	switch (lone_lisp_type_of(argument)) {
	case LONE_LISP_TAG_NIL:
		/* nothing to import: (import ()) */ linux_exit(-1);
	case LONE_LISP_TAG_FALSE:
	case LONE_LISP_TAG_TRUE:
	case LONE_LISP_TAG_INTEGER:
		/* not a supported import argument type */ linux_exit(-1);
	case LONE_LISP_TAG_SYMBOL:
		/* (import module) */
		name = argument;
		argument = lone_lisp_nil();
		break;
	case LONE_LISP_TAG_LIST:
		/* (import (module)), (import (module symbol)) */
		name = lone_lisp_list_first(lone, argument);
		argument = lone_lisp_list_rest(lone, argument);
		break;
	case LONE_LISP_TAG_MODULE:
	case LONE_LISP_TAG_FUNCTION:
	case LONE_LISP_TAG_PRIMITIVE:
	case LONE_LISP_TAG_CONTINUATION:
	case LONE_LISP_TAG_GENERATOR:
	case LONE_LISP_TAG_TEXT:
	case LONE_LISP_TAG_BYTES:
	case LONE_LISP_TAG_VECTOR:
	case LONE_LISP_TAG_TABLE:
	default:
		/* not a supported import argument type */ linux_exit(-1);
	}

	spec->module = lone_lisp_module_load(lone, name);
	if (lone_lisp_is_nil(spec->module)) { /* module not found: (import (non-existent)) */ linux_exit(-1); }

	spec->symbols = lone_lisp_is_nil(argument)?
		lone_lisp_heap_value_of(lone, spec->module)->as.module.exports : lone_lisp_list_to_vector(lone, argument);

	lone_lisp_import_specification(lone, spec);
}

LONE_LISP_PRIMITIVE(module_import)
{
	struct lone_lisp_import_specification spec;
	struct lone_lisp_value arguments, argument, prefixed, unprefixed;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	if (lone_lisp_is_nil(arguments)) { /* nothing to import: (import) */ linux_exit(-1); }

	prefixed = lone_lisp_intern_c_string(lone, "prefixed");
	unprefixed = lone_lisp_intern_c_string(lone, "unprefixed");

	spec.environment = machine->environment;
	spec.prefixed = false;

	for (/* argument */; !lone_lisp_is_nil(arguments); arguments = lone_lisp_list_rest(lone, arguments)) {
		argument = lone_lisp_list_first(lone, arguments);
		if (lone_lisp_is_list(lone, argument)) {
			lone_lisp_primitive_import_form(lone, &spec, argument);
		} else if (lone_lisp_is_symbol(lone, argument)) {
			if (lone_lisp_is_equivalent(lone, argument, prefixed)) { spec.prefixed = true; }
			else if (lone_lisp_is_equivalent(lone, argument, unprefixed)) { spec.prefixed = false; }
		} else {
			/* invalid import argument */ linux_exit(-1);
		}
	}

	lone_lisp_machine_push_value(lone, machine, lone_lisp_nil());
	return 0;
}

void lone_lisp_module_path_push(struct lone_lisp *lone, struct lone_lisp_value directory)
{
	lone_lisp_vector_push(lone, lone->modules.path, directory);
}

void lone_lisp_module_path_push_c_string(struct lone_lisp *lone, char *directory)
{
	lone_lisp_module_path_push(lone, lone_lisp_text_from_c_string(lone, directory));
}

void lone_lisp_module_path_push_va_list(struct lone_lisp *lone, size_t count, va_list directories)
{
	size_t i;

	for (i = 0; i < count; ++i) {
		lone_lisp_module_path_push_c_string(lone, va_arg(directories, char *));
	}
}

void lone_lisp_module_path_push_all(struct lone_lisp *lone, size_t count, ...)
{
	va_list directories;

	va_start(directories, count);
	lone_lisp_module_path_push_va_list(lone, count, directories);
	va_end(directories);
}
