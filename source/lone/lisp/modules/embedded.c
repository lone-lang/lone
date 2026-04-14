/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/modules/embedded.h>

#include <lone/lisp/module.h>
#include <lone/lisp/segment.h>

#include <lone/lisp/reader.h>

#include <lone/linux.h>

static struct lone_bytes slice(struct lone_lisp *lone, struct lone_bytes bytes, struct lone_lisp_value pair)
{
	struct lone_lisp_value first, second;
	lone_lisp_integer start_integer, size_integer;
	size_t start, end, size;

	if (!lone_lisp_is_list(lone, pair)) { goto unexpected_value_type; }
	first = lone_lisp_list_first(lone, pair);
	if (!lone_lisp_is_integer(lone, first)) { goto unexpected_value_type; }
	second = lone_lisp_list_rest(lone, pair);
	if (!lone_lisp_is_integer(lone, second)) { goto unexpected_value_type; }

	start_integer = lone_lisp_integer_of(first);
	size_integer = lone_lisp_integer_of(second);

	if (start_integer < 0 || size_integer < 0) { goto negative_offset_or_size; }

	start = (size_t) start_integer;
	size = (size_t) size_integer;

	if (__builtin_add_overflow(start, size, &end)) { goto overflow; }

	if (start >= bytes.count || end > bytes.count) {
		/* segment overrun */ linux_exit(-1);
	}

	return (struct lone_bytes) {
		.count = size,
		.pointer = bytes.pointer + start
	};

unexpected_value_type:
negative_offset_or_size:
overflow:
	linux_exit(-1);
}

void lone_lisp_modules_embedded_load(struct lone_lisp *lone, lone_elf_native_segment *segment)
{
	struct lone_lisp_value descriptor;
	struct lone_lisp_value symbol, data, module, locations;
	struct lone_bytes bytes, code;

	descriptor = lone_lisp_segment_read_descriptor(lone, segment);

	if (lone_lisp_is_nil(descriptor)) { /* nothing to load */ return; }

	symbol = lone_lisp_intern_c_string(lone, "data");
	data = lone_lisp_table_get(lone, descriptor, symbol);
	bytes = lone_lisp_heap_value_of(lone, data)->as.bytes;

	symbol = lone_lisp_intern_c_string(lone, "modules");
	lone->modules.embedded = lone_lisp_table_get(lone, descriptor, symbol);

	symbol = lone_lisp_intern_c_string(lone, "run");
	locations = lone_lisp_table_get(lone, descriptor, symbol);

	if (lone_lisp_is_nil(locations)) { /* no code to evaluate */ return; }

	code = slice(lone, bytes, locations);

	module = lone_lisp_module_for_name(lone, symbol);
	lone_lisp_module_load_from_bytes(lone, module, code);
}
