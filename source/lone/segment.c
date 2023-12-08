#include <lone/segment.h>
#include <lone/lisp/reader.h>
#include <lone/value/bytes.h>
#include <lone/value/symbol.h>
#include <lone/value/table.h>
#include <lone/linux.h>

struct lone_bytes lone_segment_bytes(lone_elf_segment *segment)
{
	if (!segment) {
		return (struct lone_bytes) {
			.count = 0,
			.pointer = 0
		};
	}

	return (struct lone_bytes) {
		.count = segment->p_memsz,
		.pointer = (unsigned char *) segment->p_vaddr
	};
}

struct lone_value lone_segment_read_descriptor(struct lone_lisp *lone, lone_elf_segment *segment)
{
	struct lone_bytes bytes = lone_segment_bytes(segment);
	struct lone_value descriptor, symbol, data;
	struct lone_reader reader;
	size_t offset;

	if (bytes.count == 0) { /* empty lone segment */ return lone_nil(); }

	lone_reader_for_bytes(lone, &reader, bytes);
	descriptor = lone_read(lone, &reader);
	if (reader.status.error || !lone_is_table(descriptor)) {
		/* corrupt or invalid segment */ linux_exit(-1);
	}

	offset = reader.buffer.position.read;
	symbol = lone_intern_c_string(lone, "data");
	data = lone_bytes_transfer(lone, bytes.pointer + offset, bytes.count - offset, false);
	lone_table_set(lone, descriptor, symbol, data);

	return descriptor;
}
