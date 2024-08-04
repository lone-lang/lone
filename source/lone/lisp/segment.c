/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/segment.h>

#include <lone/lisp/segment.h>
#include <lone/lisp/reader.h>

#include <lone/lisp/value/bytes.h>
#include <lone/lisp/value/symbol.h>
#include <lone/lisp/value/table.h>

#include <lone/linux.h>

struct lone_lisp_value lone_lisp_segment_read_descriptor(struct lone_lisp *lone, lone_elf_native_segment *segment)
{
	struct lone_bytes bytes;
	struct lone_lisp_value descriptor, symbol, data;
	struct lone_lisp_reader reader;
	size_t offset;

	bytes = lone_segment_bytes(segment);

	if (bytes.count == 0) {
		/* empty lone segment */ return lone_lisp_nil();
	}

	lone_lisp_reader_for_bytes(lone, &reader, bytes);
	descriptor = lone_lisp_read(lone, &reader);
	if (reader.status.error || !lone_lisp_is_table(descriptor)) {
		/* corrupt or invalid segment */ linux_exit(-1);
	}

	offset = reader.buffer.position.read;
	symbol = lone_lisp_intern_c_string(lone, "data");
	data = lone_lisp_bytes_transfer(lone, bytes.pointer + offset, bytes.count - offset, false);
	lone_lisp_table_set(lone, descriptor, symbol, data);

	return descriptor;
}
