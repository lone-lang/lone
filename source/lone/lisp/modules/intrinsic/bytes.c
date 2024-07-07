/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/modules/intrinsic/bytes.h>

#include <lone/lisp/value/primitive.h>
#include <lone/lisp/value/list.h>
#include <lone/lisp/value/table.h>
#include <lone/lisp/value/symbol.h>
#include <lone/lisp/value/bytes.h>
#include <lone/lisp/value/integer.h>

#include <lone/lisp/module.h>

#include <lone/memory/allocator.h>

#include <lone/linux.h>

void lone_lisp_modules_intrinsic_bytes_initialize(struct lone_lisp *lone)
{
	struct lone_lisp_function_flags flags;
	struct lone_lisp_value name, module;

	name = lone_lisp_intern_c_string(lone, "bytes");
	module = lone_lisp_module_for_name(lone, name);

	flags = (struct lone_lisp_function_flags) { .evaluate_arguments = true, .evaluate_result = false };

	lone_lisp_module_export_primitive(lone, module, "new",
			"bytes_new", lone_lisp_primitive_bytes_new, module, flags);

#define LONE_LISP_EXPORT_BYTES_READER_PRIMITIVE(type) \
	lone_lisp_module_export_primitive(lone, module, "read-" #type, \
			"bytes_read_" #type, lone_lisp_primitive_bytes_read_##type, module, flags)

#define LONE_LISP_EXPORT_BYTES_WRITER_PRIMITIVE(type) \
	lone_lisp_module_export_primitive(lone, module, "write-" #type, \
			"bytes_write_" #type, lone_lisp_primitive_bytes_write_##type, module, flags)

	LONE_LISP_EXPORT_BYTES_READER_PRIMITIVE(u8);
	LONE_LISP_EXPORT_BYTES_READER_PRIMITIVE(s8);
	LONE_LISP_EXPORT_BYTES_READER_PRIMITIVE(u16);
	LONE_LISP_EXPORT_BYTES_READER_PRIMITIVE(s16);
	LONE_LISP_EXPORT_BYTES_READER_PRIMITIVE(u32);
	LONE_LISP_EXPORT_BYTES_READER_PRIMITIVE(s32);

	LONE_LISP_EXPORT_BYTES_WRITER_PRIMITIVE(u8);
	LONE_LISP_EXPORT_BYTES_WRITER_PRIMITIVE(s8);
	LONE_LISP_EXPORT_BYTES_WRITER_PRIMITIVE(u16);
	LONE_LISP_EXPORT_BYTES_WRITER_PRIMITIVE(s16);
	LONE_LISP_EXPORT_BYTES_WRITER_PRIMITIVE(u32);
	LONE_LISP_EXPORT_BYTES_WRITER_PRIMITIVE(s32);

#undef LONE_LISP_EXPORT_BYTES_READER_PRIMITIVE
#undef LONE_LISP_EXPORT_BYTES_WRITER_PRIMITIVE
}

LONE_LISP_PRIMITIVE(bytes_new)
{
	struct lone_lisp_value count;
	size_t allocation;

	if (lone_lisp_list_destructure(arguments, 1, &count)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	switch (count.type) {
	case LONE_LISP_TYPE_INTEGER:
		if (count.as.integer <= 0) {
			/* zero or negative allocation, likely a mistake: (new 0), (new -64) */ linux_exit(-1);
		}

		allocation = count.as.integer;
		break;
	case LONE_LISP_TYPE_NIL:
	case LONE_LISP_TYPE_POINTER:
	case LONE_LISP_TYPE_HEAP_VALUE:
		/* count not an integer: (new {}) */ linux_exit(-1);
	}

	return lone_lisp_bytes_create(lone, allocation);
}

static void lone_lisp_bytes_check_read_arguments(struct lone_lisp_value bytes,
		struct lone_lisp_value offset)
{
	if (!lone_lisp_is_bytes(bytes)) {
		/* invalid data source: (reader "text" 0), (reader {} 0) */ linux_exit(-1);
	}

	if (!lone_lisp_is_integer(offset)) {
		/* invalid offset: (reader bytes "invalid"), (reader bytes []) */ linux_exit(-1);
	}
}

static void lone_lisp_bytes_check_write_arguments(struct lone_lisp_value bytes,
		struct lone_lisp_value offset, struct lone_lisp_value value)
{
	lone_lisp_bytes_check_read_arguments(bytes, offset);

	if (!lone_lisp_is_integer(value)) {
		/* invalid value: (writer bytes 0 "invalid"), (writer bytes 0 []) */ linux_exit(-1);
	}
}

#define LONE_LISP_BYTES_READER_PRIMITIVE(type) \
LONE_LISP_PRIMITIVE(bytes_read_##type) \
{ \
	struct lone_lisp_value bytes; \
	struct lone_lisp_value offset; \
\
	struct lone_##type type; \
\
	if (lone_lisp_list_destructure(arguments, 2, &bytes, &offset)) { \
		/* wrong number of arguments */ linux_exit(-1); \
	} \
\
	lone_lisp_bytes_check_read_arguments(bytes, offset); \
\
	type = lone_bytes_read_##type(bytes.as.heap_value->as.bytes, offset.as.integer); \
\
	if (type.present) { \
		return lone_lisp_integer_create(type.value); \
	} else { \
		linux_exit(-1); \
	} \
}

#define LONE_LISP_BYTES_WRITER_PRIMITIVE(type) \
LONE_LISP_PRIMITIVE(bytes_write_##type) \
{ \
	struct lone_lisp_value bytes; \
	struct lone_lisp_value offset; \
	struct lone_lisp_value value; \
\
	lone_##type type; \
	bool success; \
\
	if (lone_lisp_list_destructure(arguments, 3, &bytes, &offset, &value)) { \
		/* wrong number of arguments */ linux_exit(-1); \
	} \
\
	lone_lisp_bytes_check_write_arguments(bytes, offset, value); \
\
	type = (lone_##type) value.as.integer; \
\
	success = lone_bytes_write_##type( \
		bytes.as.heap_value->as.bytes, \
		offset.as.integer, \
		type \
	); \
\
	if (success) { \
		return lone_lisp_integer_create(type); \
	} else { \
		linux_exit(-1); \
	} \
}

LONE_LISP_BYTES_READER_PRIMITIVE(u8)
LONE_LISP_BYTES_READER_PRIMITIVE(s8)
LONE_LISP_BYTES_READER_PRIMITIVE(u16)
LONE_LISP_BYTES_READER_PRIMITIVE(s16)
LONE_LISP_BYTES_READER_PRIMITIVE(u32)
LONE_LISP_BYTES_READER_PRIMITIVE(s32)

LONE_LISP_BYTES_WRITER_PRIMITIVE(u8)
LONE_LISP_BYTES_WRITER_PRIMITIVE(s8)
LONE_LISP_BYTES_WRITER_PRIMITIVE(u16)
LONE_LISP_BYTES_WRITER_PRIMITIVE(s16)
LONE_LISP_BYTES_WRITER_PRIMITIVE(u32)
LONE_LISP_BYTES_WRITER_PRIMITIVE(s32)

#undef LONE_LISP_BYTES_READER_PRIMITIVE
#undef LONE_LISP_BYTES_WRITER_PRIMITIVE
