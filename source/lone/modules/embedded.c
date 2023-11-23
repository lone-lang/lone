#include <lone/definitions.h>
#include <lone/types.h>

#include <lone/modules/embedded.h>
#include <lone/modules.h>
#include <lone/utilities.h>
#include <lone/linux.h>

#include <lone/lisp/reader.h>
#include <lone/value/list.h>
#include <lone/value/table.h>
#include <lone/value/symbol.h>

static void lone_load_segment_modules(struct lone_lisp *lone, struct lone_bytes bytes)
{
	struct lone_reader reader;
	struct lone_value *descriptor;
	struct lone_value *run, *locations, *first, *rest;
	size_t offset, start, end, size, i;
	struct lone_bytes code;

	lone_reader_for_bytes(lone, &reader, bytes);
	descriptor = lone_read(lone, &reader);
	if (!lone_is_table(descriptor)) { /* empty or corrupt segment */ linux_exit(-1); }

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

static void lone_load_segment(struct lone_lisp *lone, lone_elf_segment *segment)
{
	struct lone_bytes bytes = {
		.count = segment->p_memsz,
		.pointer = (unsigned char *) segment->p_vaddr
	};

	lone_load_segment_modules(lone, bytes);
}

void lone_modules_embedded_load(struct lone_lisp *lone, struct lone_auxiliary_vector *values)
{
	struct lone_elf_segments table = lone_auxiliary_vector_elf_segments(values);

	for (size_t i = 0; i < table.entry_count; ++i) {
		lone_elf_segment *segment = &table.segments[i];

		if (segment->p_type == PT_LONE) {
			lone_load_segment(lone, segment);
			break;
		}
	}
}
