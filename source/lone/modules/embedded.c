#include <lone/definitions.h>
#include <lone/types.h>

#include <lone/modules/embedded.h>
#include <lone/modules.h>
#include <lone/segment.h>
#include <lone/linux.h>

#include <lone/lisp/reader.h>
#include <lone/value/list.h>
#include <lone/value/table.h>
#include <lone/value/symbol.h>

void lone_modules_embedded_load(struct lone_lisp *lone, lone_elf_segment *segment)
{
	struct lone_value *descriptor = lone_segment_read_descriptor(lone, segment);
	struct lone_value *module, *symbol, *data, *locations, *first, *rest;
	struct lone_bytes bytes, code;
	size_t start, end;

	if (!descriptor) { /* nothing to load */ return; }

	symbol = lone_intern_c_string(lone, "data");
	data = lone_table_get(lone, descriptor, symbol);
	bytes = data->bytes;

	symbol = lone_intern_c_string(lone, "run");
	locations = lone_table_get(lone, descriptor, symbol);
	if (!lone_is_list(locations)) { /* unexpected value type */ linux_exit(-1); }
	first = lone_list_first(locations);
	if (!lone_is_integer(first)) { /* unexpected value type */ linux_exit(-1); }
	rest = lone_list_rest(locations);
	if (!lone_is_integer(rest)) { /* unexpected value type */ linux_exit(-1); }

	start = first->integer;
	end = rest->integer;

	if (start >= bytes.count || end >= bytes.count) {
		/* segment overrun */ linux_exit(-1);
	}

	code = (struct lone_bytes) { .count = end - start, .pointer = bytes.pointer + start };

	module = lone_module_for_name(lone, symbol);
	lone_module_load_from_bytes(lone, module, code);
}
