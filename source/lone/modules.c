/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/modules.h>

#include <lone/memory/allocator.h>
#include <lone/memory/garbage_collector.h>

#include <lone/linux.h>
#include <lone/lisp.h>
#include <lone/lisp/reader.h>
#include <lone/lisp/evaluator.h>

#include <lone/value/module.h>
#include <lone/value/list.h>
#include <lone/value/vector.h>
#include <lone/value/table.h>
#include <lone/value/text.h>
#include <lone/value/symbol.h>

#include <lone/struct/lisp.h>
#include <lone/struct/reader.h>
#include <lone/struct/value.h>

struct lone_value *lone_module_null(struct lone_lisp *lone)
{
	return lone->modules.null;
}

static struct lone_value *lone_module_name_to_key(struct lone_lisp *lone, struct lone_value *name)
{
	struct lone_value *head;

	switch (name->type) {
	case LONE_SYMBOL:
		return lone_list_create(lone, name, lone_nil(lone));
	case LONE_LIST:
		for (head = name; !lone_is_nil(head); head = lone_list_rest(head)) {
			if (!lone_is_symbol(lone_list_first(head))) {
				linux_exit(-1);
			}
		}
		return name;
	case LONE_MODULE:
		return lone_module_name_to_key(lone, name->module.name);
	case LONE_TEXT: case LONE_BYTES:
	case LONE_FUNCTION: case LONE_PRIMITIVE:
	case LONE_VECTOR: case LONE_TABLE:
	case LONE_INTEGER: case LONE_POINTER:
		linux_exit(-1);
	}
}

static struct lone_value *lone_module_get_or_create(struct lone_lisp *lone, struct lone_value *name, bool *not_found)
{
	struct lone_value *module;

	name = lone_module_name_to_key(lone, name);
	module = lone_table_get(lone, lone->modules.loaded, name);
	if (not_found) {
		*not_found = false;
	}

	if (lone_is_nil(module)) {
		module = lone_module_create(lone, name);
		lone_table_set(lone, lone->modules.loaded, name, module);
		if (not_found) {
			*not_found = true;
		}
	}

	return module;
}

struct lone_value *lone_module_for_name(struct lone_lisp *lone, struct lone_value *name)
{
	return lone_module_get_or_create(lone, name, 0);
}

static int lone_module_search(struct lone_lisp *lone, struct lone_value *symbols)
{
	struct lone_value *slash = lone_intern_c_string(lone, "/"), *ln = lone_intern_c_string(lone, ".ln");
	struct lone_value *arguments, *package, *search_path;
	unsigned char *path;
	size_t i;
	long result;

	symbols = lone_module_name_to_key(lone, symbols);
	package = lone_list_first(symbols);

	for (i = 0; i < lone->modules.path->vector.count; ++i) {
		search_path = lone->modules.path->vector.values[i];
		arguments = lone_list_build(lone, 3, search_path, package, symbols);
		arguments = lone_list_flatten(lone, arguments);
		arguments = lone_text_transfer_bytes(lone, lone_join(lone, slash, arguments, lone_has_bytes), true);
		arguments = lone_list_build(lone, 2, arguments, ln);
		path = lone_concatenate(lone, arguments, lone_has_bytes).pointer;

		result = linux_openat(AT_FDCWD, path, O_RDONLY | O_CLOEXEC);

		lone_deallocate(lone, path);

		switch (result) {
		case -ENOENT:
		case -EACCES: case -EPERM:
		case -ENOTDIR: case -EISDIR:
		case -EINVAL: case -ENAMETOOLONG:
		case -EMFILE: case -ENFILE:
		case -ELOOP:
			continue;
		case -ENOMEM: case -EFAULT:
			linux_exit(-1);
		}

		return (int) result;
	}

	linux_exit(-1); /* module not found */
}

static void lone_module_load_from_reader(struct lone_lisp *lone, struct lone_value *module, struct lone_reader *reader)
{
	struct lone_value *value;

	while (1) {
		value = lone_read(lone, reader);
		if (!value) { if (reader->error) { linux_exit(-1); } else { break; } }

		value = lone_evaluate_module(lone, module, value);
	}

	lone_reader_finalize(lone, reader);
	lone_garbage_collector(lone);
}

void lone_module_load_from_bytes(struct lone_lisp *lone, struct lone_value *module, struct lone_bytes bytes)
{
	struct lone_reader reader;

	lone_reader_for_bytes(lone, &reader, bytes);
	lone_module_load_from_reader(lone, module, &reader);
}

static void lone_module_load_from_file_descriptor(struct lone_lisp *lone, struct lone_value *module, int file_descriptor)
{
	struct lone_reader reader;

	lone_reader_for_file_descriptor(lone, &reader, LONE_BUFFER_SIZE, file_descriptor);
	lone_module_load_from_reader(lone, module, &reader);
}

struct lone_value *lone_module_load(struct lone_lisp *lone, struct lone_value *name)
{
	struct lone_value *module;
	bool not_found;
	int file_descriptor;

	module = lone_module_get_or_create(lone, name, &not_found);

	if (not_found) {
		file_descriptor = lone_module_search(lone, name);
		lone_module_load_from_file_descriptor(lone, module, file_descriptor);
		linux_close(file_descriptor);
	}

	return module;
}

void lone_module_load_null_from_file_descriptor(struct lone_lisp *lone, int file_descriptor)
{
	lone_module_load_from_file_descriptor(lone, lone_module_null(lone), file_descriptor);
}

void lone_module_load_null_from_standard_input(struct lone_lisp *lone)
{
	lone_module_load_null_from_file_descriptor(lone, 0);
}

void lone_export(struct lone_lisp *lone, struct lone_value *module, struct lone_value *symbol)
{
	if (!lone_is_symbol(symbol)) { /* only symbols can be exported */ linux_exit(-1); }
	lone_vector_push(lone, module->module.exports, symbol);
}

void lone_set_and_export(struct lone_lisp *lone, struct lone_value *module, struct lone_value *symbol, struct lone_value *value)
{
	lone_export(lone, module, symbol);
	lone_table_set(lone, module->module.environment, symbol, value);
}

struct lone_value *lone_primitive_export(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *head, *symbol;

	for (head = arguments; !lone_is_nil(head); head = lone_list_rest(head)) {
		symbol = lone_list_first(head);

		lone_export(lone, module, symbol);
	}

	return lone_nil(lone);
}

struct lone_import_specification {
	struct lone_value *module;         /* module value to import from */
	struct lone_value *symbols;        /* list of symbols to import */
	struct lone_value *environment;    /* environment to import symbols to */

	bool prefixed;                     /* whether to prefix symbols */
};

static struct lone_value *lone_prefix_module_name(struct lone_lisp *lone, struct lone_value *module, struct lone_value *symbol)
{
	struct lone_value *arguments = lone_list_flatten(lone, lone_list_build(lone, 2, module->module.name, symbol)),
	                  *dot = lone_intern_c_string(lone, ".");

	return lone_symbol_transfer_bytes(lone, lone_join(lone, dot, arguments, lone_has_bytes), true);
}

static void lone_import_specification(struct lone_lisp *lone, struct lone_import_specification *spec)
{
	size_t i;
	struct lone_value *module = spec->module, *symbols = spec->symbols, *environment = spec->environment, *exports = module->module.exports,
	                  *symbol, *value;

	/* bind either the exported or the specified symbols: (import (module)), (import (module x f)) */
	for (i = 0; i < symbols->vector.count; ++i) {
		symbol = lone_vector_get_value_at(lone, symbols, i);
		if (!lone_is_symbol(symbol)) { /* name not a symbol: (import (module 10)) */ linux_exit(-1); }

		if (symbols != exports && !lone_vector_contains(exports, symbol)) {
			/* attempt to import private symbol */ linux_exit(-1);
		}

		value = lone_table_get(lone, module->module.environment, symbol);

		if (spec->prefixed) {
			symbol = lone_prefix_module_name(lone, spec->module, symbol);
		}

		lone_table_set(lone, environment, symbol, value);
	}
}

static void lone_primitive_import_form(struct lone_lisp *lone, struct lone_import_specification *spec, struct lone_value *argument)
{
	struct lone_value *name;

	if (lone_is_nil(argument)) { /* nothing to import: (import ()) */ linux_exit(-1); }

	switch (argument->type) {
	case LONE_SYMBOL:
		/* (import module) */
		name = argument;
		argument = lone_nil(lone);
		break;
	case LONE_LIST:
		/* (import (module)), (import (module symbol)) */
		name = lone_list_first(argument);
		argument = lone_list_rest(argument);
		break;
	case LONE_MODULE:
	case LONE_FUNCTION: case LONE_PRIMITIVE:
	case LONE_TEXT: case LONE_BYTES:
	case LONE_VECTOR: case LONE_TABLE:
	case LONE_INTEGER: case LONE_POINTER:
		/* not a supported import argument type */ linux_exit(-1);
	}

	spec->module = lone_module_load(lone, name);
	if (lone_is_nil(spec->module)) { /* module not found: (import non-existent), (import (non-existent)) */ linux_exit(-1); }

	spec->symbols = lone_is_nil(argument)? spec->module->module.exports : lone_list_to_vector(lone, argument);

	lone_import_specification(lone, spec);
}

struct lone_value *lone_primitive_import(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_import_specification spec;
	struct lone_value *prefixed = lone_intern_c_string(lone, "prefixed"),
	                  *unprefixed = lone_intern_c_string(lone, "unprefixed"),
	                  *argument;

	if (lone_is_nil(arguments)) { /* nothing to import: (import) */ linux_exit(-1); }

	spec.environment = environment;
	spec.prefixed = false;

	for (/* argument */; !lone_is_nil(arguments); arguments = lone_list_rest(arguments)) {
		argument = lone_list_first(arguments);
		if (lone_is_list(argument)) {
			lone_primitive_import_form(lone, &spec, argument);
		} else if (lone_is_symbol(argument)) {
			if (lone_is_equivalent(argument, prefixed)) { spec.prefixed = true; }
			else if (lone_is_equivalent(argument, unprefixed)) { spec.prefixed = false; }
		} else {
			/* invalid import argument */ linux_exit(-1);
		}
	}

	return lone_nil(lone);
}

void lone_module_path_push(struct lone_lisp *lone, struct lone_value *directory)
{
	lone_vector_push(lone, lone->modules.path, directory);
}

void lone_module_path_push_c_string(struct lone_lisp *lone, char *directory)
{
	lone_module_path_push(lone, lone_text_create_from_c_string(lone, directory));
}

void lone_module_path_push_va_list(struct lone_lisp *lone, size_t count, va_list directories)
{
	struct lone_value *directory;
	size_t i;

	for (i = 0; i < count; ++i) {
		directory = va_arg(directories, struct lone_value *);
		lone_module_path_push(lone, directory);
	}
}

void lone_module_path_push_all(struct lone_lisp *lone, size_t count, ...)
{
	va_list directories;

	va_start(directories, count);
	lone_module_path_push_va_list(lone, count, directories);
	va_end(directories);
}
