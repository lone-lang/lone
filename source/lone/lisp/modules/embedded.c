/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/modules/embedded.h>

#include <lone/lisp/module.h>
#include <lone/lisp/segment.h>

#include <lone/lisp/reader.h>
#include <lone/lisp/value/list.h>
#include <lone/lisp/value/table.h>
#include <lone/lisp/value/symbol.h>

#include <lone/linux.h>

static struct lone_bytes slice(struct lone_bytes bytes, struct lone_lisp_value pair)
{
	struct lone_lisp_value first, second;
	size_t start, end, size;

	if (!lone_lisp_is_list(pair)) { /* unexpected value type */ linux_exit(-1); }
	first = lone_lisp_list_first(pair);
	if (!lone_lisp_is_integer(first)) { /* unexpected value type */ linux_exit(-1); }
	second = lone_lisp_list_rest(pair);
	if (!lone_lisp_is_integer(second)) { /* unexpected value type */ linux_exit(-1); }

	start = lone_lisp_value_to_integer(first);
	size = lone_lisp_value_to_integer(second);
	end = start + size;

	if (start >= bytes.count || end >= bytes.count) {
		/* segment overrun */ linux_exit(-1);
	}

	return (struct lone_bytes) {
		.count = size,
		.pointer = bytes.pointer + start
	};
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
	bytes = lone_lisp_value_to_heap_value(data)->as.bytes;

	symbol = lone_lisp_intern_c_string(lone, "modules");
	lone->modules.embedded = lone_lisp_table_get(lone, descriptor, symbol);

	symbol = lone_lisp_intern_c_string(lone, "run");
	locations = lone_lisp_table_get(lone, descriptor, symbol);

	if (lone_lisp_is_nil(locations)) { /* no code to evaluate */ return; }

	code = slice(bytes, locations);

	module = lone_lisp_module_for_name(lone, symbol);
	lone_lisp_module_load_from_bytes(lone, module, code);
}
