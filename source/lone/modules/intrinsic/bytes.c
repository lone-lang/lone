/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/modules/intrinsic/bytes.h>
#include <lone/modules.h>
#include <lone/memory/allocator.h>
#include <lone/linux.h>

#include <lone/value/primitive.h>
#include <lone/value/list.h>
#include <lone/value/table.h>
#include <lone/value/symbol.h>
#include <lone/value/bytes.h>
#include <lone/value/integer.h>

void lone_modules_intrinsic_bytes_initialize(struct lone_lisp *lone)
{
	struct lone_value name = lone_intern_c_string(lone, "bytes"),
	                  module = lone_module_for_name(lone, name),
	                  primitive;

	struct lone_function_flags flags = { .evaluate_arguments = true, .evaluate_result = false };

	primitive = lone_primitive_create(lone, "bytes_new", lone_primitive_bytes_new, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "new"), primitive);

#define LONE_REGISTER_BYTES_READER_PRIMITIVE(type) \
	primitive = lone_primitive_create(lone, "bytes_read_" #type, lone_primitive_bytes_read_##type, module, flags); \
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "read-" #type), primitive)

#define LONE_REGISTER_BYTES_WRITER_PRIMITIVE(type) \
	primitive = lone_primitive_create(lone, "bytes_write_" #type, lone_primitive_bytes_write_##type, module, flags); \
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "write-" #type), primitive)

	LONE_REGISTER_BYTES_READER_PRIMITIVE(u8);
	LONE_REGISTER_BYTES_READER_PRIMITIVE(s8);
	LONE_REGISTER_BYTES_READER_PRIMITIVE(u16);
	LONE_REGISTER_BYTES_READER_PRIMITIVE(s16);
	LONE_REGISTER_BYTES_READER_PRIMITIVE(u32);
	LONE_REGISTER_BYTES_READER_PRIMITIVE(s32);

	LONE_REGISTER_BYTES_WRITER_PRIMITIVE(u8);
	LONE_REGISTER_BYTES_WRITER_PRIMITIVE(s8);
	LONE_REGISTER_BYTES_WRITER_PRIMITIVE(u16);
	LONE_REGISTER_BYTES_WRITER_PRIMITIVE(s16);
	LONE_REGISTER_BYTES_WRITER_PRIMITIVE(u32);
	LONE_REGISTER_BYTES_WRITER_PRIMITIVE(s32);

#undef LONE_REGISTER_BYTES_READER_PRIMITIVE
#undef LONE_REGISTER_BYTES_WRITER_PRIMITIVE

	lone_table_set(lone, lone->modules.loaded, name, module);
}

LONE_PRIMITIVE(bytes_new)
{
	struct lone_value count;
	size_t allocation;

	if (lone_list_destructure(arguments, 1, &count)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	switch (count.type) {
	case LONE_INTEGER:
		if (count.as.signed_integer <= 0) {
			/* zero or negative allocation, likely a mistake: (new 0), (new -64) */ linux_exit(-1);
		}

		allocation = count.as.unsigned_integer;
		break;
	case LONE_NIL:
	case LONE_POINTER:
	case LONE_HEAP_VALUE:
		/* count not an integer: (new {}) */ linux_exit(-1);
	}

	return lone_bytes_create(lone, allocation);
}

static void lone_bytes_check_read_arguments(struct lone_value bytes, struct lone_value offset)
{
	if (!lone_is_bytes(bytes)) {
		/* invalid data source: (reader "text" 0), (reader {} 0) */ linux_exit(-1);
	}

	if (!lone_is_integer(offset)) {
		/* invalid offset: (reader bytes "invalid"), (reader bytes []) */ linux_exit(-1);
	}
}

static void lone_bytes_check_write_arguments(struct lone_value bytes, struct lone_value offset, struct lone_value value)
{
	lone_bytes_check_read_arguments(bytes, offset);

	if (!lone_is_integer(value)) {
		/* invalid value: (writer bytes 0 "invalid"), (writer bytes 0 []) */ linux_exit(-1);
	}
}

#define LONE_BYTES_READER_PRIMITIVE(type) \
LONE_PRIMITIVE(bytes_read_##type) \
{ \
	struct lone_value bytes; \
	struct lone_value offset; \
\
	lone_##type type; \
	bool success; \
\
	if (lone_list_destructure(arguments, 2, &bytes, &offset)) { \
		/* wrong number of arguments */ linux_exit(-1); \
	} \
\
	lone_bytes_check_read_arguments(bytes, offset); \
\
	success = lone_bytes_checked_read_##type( \
		bytes.as.heap_value->as.bytes, \
		offset.as.unsigned_integer, \
		&type \
	); \
\
	if (success) { \
		return lone_integer_create(type); \
	} else { \
		linux_exit(-1); \
	} \
}

#define LONE_BYTES_WRITER_PRIMITIVE(type) \
LONE_PRIMITIVE(bytes_write_##type) \
{ \
	struct lone_value bytes; \
	struct lone_value offset; \
	struct lone_value value; \
\
	lone_##type type; \
	bool success; \
\
	if (lone_list_destructure(arguments, 3, &bytes, &offset, &value)) { \
		/* wrong number of arguments */ linux_exit(-1); \
	} \
\
	lone_bytes_check_write_arguments(bytes, offset, value); \
\
	type = (lone_##type) value.as.unsigned_integer; \
\
	success = lone_bytes_checked_write_##type( \
		bytes.as.heap_value->as.bytes, \
		offset.as.unsigned_integer, \
		type \
	); \
\
	if (success) { \
		return lone_integer_create(type); \
	} else { \
		linux_exit(-1); \
	} \
}

LONE_BYTES_READER_PRIMITIVE(u8)
LONE_BYTES_READER_PRIMITIVE(s8)
LONE_BYTES_READER_PRIMITIVE(u16)
LONE_BYTES_READER_PRIMITIVE(s16)
LONE_BYTES_READER_PRIMITIVE(u32)
LONE_BYTES_READER_PRIMITIVE(s32)

LONE_BYTES_WRITER_PRIMITIVE(u8)
LONE_BYTES_WRITER_PRIMITIVE(s8)
LONE_BYTES_WRITER_PRIMITIVE(u16)
LONE_BYTES_WRITER_PRIMITIVE(s16)
LONE_BYTES_WRITER_PRIMITIVE(u32)
LONE_BYTES_WRITER_PRIMITIVE(s32)

#undef LONE_BYTES_READER_PRIMITIVE
#undef LONE_BYTES_WRITER_PRIMITIVE
