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

static void lone_load_segment(struct lone_lisp *lone, struct lone_bytes bytes)
{
	struct lone_reader reader;
	struct lone_value *descriptor;
	struct lone_value *run, *locations, *first, *rest;
	size_t offset, start, end, size, i;
	struct lone_bytes code;

	if (bytes.count == 0) { /* empty lone segment */ return; }

	lone_reader_for_bytes(lone, &reader, bytes);
	descriptor = lone_read(lone, &reader);
	if (!descriptor || !lone_is_table(descriptor)) { /* empty or corrupt segment */ linux_exit(-1); }

	offset = reader.buffer.position.read;
	run = lone_intern_c_string(lone, "run");

	locations = lone_table_get(lone, descriptor, run);
	if (!lone_is_list(locations)) { /* unexpected value type */ linux_exit(-1); }
	first = lone_list_first(locations);
	if (!lone_is_integer(first)) { /* unexpected value type */ linux_exit(-1); }
	rest = lone_list_rest(locations);
	if (!lone_is_integer(rest)) { /* unexpected value type */ linux_exit(-1); }

	start = first->integer;
	end = rest->integer;
	size = end - start;

	if (offset + size > bytes.count) { /* segment overrun */ linux_exit(-1); }
	code = (struct lone_bytes) { .count = size, .pointer = bytes.pointer + offset };

	struct lone_value *module = lone_module_for_name(lone, run);
	lone_module_load_from_bytes(lone, module, code);
}

void lone_modules_embedded_load(struct lone_lisp *lone, lone_elf_segment *segment)
{
	struct lone_bytes bytes = lone_segment_bytes(segment);
	lone_load_segment(lone, bytes);
}
