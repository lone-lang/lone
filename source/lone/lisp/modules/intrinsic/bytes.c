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

	lone_lisp_module_export_primitive(lone, module, "zero?",
			"bytes_is_zero", lone_lisp_primitive_bytes_is_zero, module, flags);

#define LONE_LISP_EXPORT_BYTES_READER_PRIMITIVE(sign, bits, endian) \
	lone_lisp_module_export_primitive(lone, module, "read-" #sign #bits #endian, \
			"bytes_read_" #sign #bits #endian, \
			lone_lisp_primitive_bytes_read_##sign##bits##endian, \
			module, flags)

#define LONE_LISP_EXPORT_BYTES_WRITER_PRIMITIVE(sign, bits, endian) \
	lone_lisp_module_export_primitive(lone, module, "write-" #sign #bits #endian, \
			"bytes_write_" #sign #bits #endian, \
			lone_lisp_primitive_bytes_write_##sign##bits##endian, \
			module, flags)

	LONE_LISP_EXPORT_BYTES_READER_PRIMITIVE(u, 8, /* no endianness */);
	LONE_LISP_EXPORT_BYTES_READER_PRIMITIVE(s, 8, /* no endianness */);

	LONE_LISP_EXPORT_BYTES_WRITER_PRIMITIVE(u, 8, /* no endianness */);
	LONE_LISP_EXPORT_BYTES_WRITER_PRIMITIVE(s, 8, /* no endianness */);

	LONE_LISP_EXPORT_BYTES_READER_PRIMITIVE(u, 16, /* native endianness */);
	LONE_LISP_EXPORT_BYTES_READER_PRIMITIVE(s, 16, /* native endianness */);
	LONE_LISP_EXPORT_BYTES_READER_PRIMITIVE(u, 32, /* native endianness */);
	LONE_LISP_EXPORT_BYTES_READER_PRIMITIVE(s, 32, /* native endianness */);

	LONE_LISP_EXPORT_BYTES_WRITER_PRIMITIVE(u, 16, /* native endianness */);
	LONE_LISP_EXPORT_BYTES_WRITER_PRIMITIVE(s, 16, /* native endianness */);
	LONE_LISP_EXPORT_BYTES_WRITER_PRIMITIVE(u, 32, /* native endianness */);
	LONE_LISP_EXPORT_BYTES_WRITER_PRIMITIVE(s, 32, /* native endianness */);

	LONE_LISP_EXPORT_BYTES_READER_PRIMITIVE(u, 16, le);
	LONE_LISP_EXPORT_BYTES_READER_PRIMITIVE(s, 16, le);
	LONE_LISP_EXPORT_BYTES_READER_PRIMITIVE(u, 32, le);
	LONE_LISP_EXPORT_BYTES_READER_PRIMITIVE(s, 32, le);

	LONE_LISP_EXPORT_BYTES_WRITER_PRIMITIVE(u, 16, le);
	LONE_LISP_EXPORT_BYTES_WRITER_PRIMITIVE(s, 16, le);
	LONE_LISP_EXPORT_BYTES_WRITER_PRIMITIVE(u, 32, le);
	LONE_LISP_EXPORT_BYTES_WRITER_PRIMITIVE(s, 32, le);

	LONE_LISP_EXPORT_BYTES_READER_PRIMITIVE(u, 16, be);
	LONE_LISP_EXPORT_BYTES_READER_PRIMITIVE(s, 16, be);
	LONE_LISP_EXPORT_BYTES_READER_PRIMITIVE(u, 32, be);
	LONE_LISP_EXPORT_BYTES_READER_PRIMITIVE(s, 32, be);

	LONE_LISP_EXPORT_BYTES_WRITER_PRIMITIVE(u, 16, be);
	LONE_LISP_EXPORT_BYTES_WRITER_PRIMITIVE(s, 16, be);
	LONE_LISP_EXPORT_BYTES_WRITER_PRIMITIVE(u, 32, be);
	LONE_LISP_EXPORT_BYTES_WRITER_PRIMITIVE(s, 32, be);

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

	switch (lone_lisp_value_to_type(count)) {
	case LONE_LISP_TYPE_INTEGER:
		if (lone_lisp_value_to_integer(count) <= 0) {
			/* zero or negative allocation, likely a mistake: (new 0), (new -64) */ linux_exit(-1);
		}

		allocation = lone_lisp_value_to_integer(count);
		break;
	case LONE_LISP_TYPE_NIL:
	case LONE_LISP_TYPE_HEAP_VALUE:
		/* count not an integer: (new {}) */ linux_exit(-1);
	}

	return lone_lisp_bytes_create(lone, allocation);
}

LONE_LISP_PRIMITIVE(bytes_is_zero)
{
	struct lone_lisp_value bytes;

	if (lone_lisp_list_destructure(arguments, 1, &bytes)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_lisp_is_bytes(bytes)) {
		/* expected a bytes object: (zero? 0), (zero? "text") */ linux_exit(-1);
	}

	return lone_lisp_boolean_for(lone, lone_bytes_is_zero(lone_lisp_value_to_heap_value(bytes)->as.bytes));
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

#define LONE_LISP_BYTES_READER_PRIMITIVE(sign, bits, endian) \
LONE_LISP_PRIMITIVE(bytes_read_##sign##bits##endian) \
{ \
	struct lone_lisp_value bytes; \
	struct lone_lisp_value offset; \
\
	struct lone_optional_##sign##bits integer; \
\
	if (lone_lisp_list_destructure(arguments, 2, &bytes, &offset)) { \
		/* wrong number of arguments */ linux_exit(-1); \
	} \
\
	lone_lisp_bytes_check_read_arguments(bytes, offset); \
\
	integer = lone_bytes_read_##sign##bits##endian(lone_lisp_value_to_heap_value(bytes)->as.bytes, \
			lone_lisp_value_to_integer(offset)); \
\
	if (integer.present) { \
		return lone_lisp_integer_create(integer.value); \
	} else { \
		linux_exit(-1); \
	} \
}

#define LONE_LISP_BYTES_WRITER_PRIMITIVE(sign, bits, endian) \
LONE_LISP_PRIMITIVE(bytes_write_##sign##bits##endian) \
{ \
	struct lone_lisp_value bytes; \
	struct lone_lisp_value offset; \
	struct lone_lisp_value value; \
\
	lone_##sign##bits integer; \
	bool success; \
\
	if (lone_lisp_list_destructure(arguments, 3, &bytes, &offset, &value)) { \
		/* wrong number of arguments */ linux_exit(-1); \
	} \
\
	lone_lisp_bytes_check_write_arguments(bytes, offset, value); \
\
	integer = (lone_##sign##bits) lone_lisp_value_to_integer(value); \
\
	success = lone_bytes_write_##sign##bits##endian( \
		lone_lisp_value_to_heap_value(bytes)->as.bytes, \
		lone_lisp_value_to_integer(offset), \
		integer \
	); \
\
	if (success) { \
		return lone_lisp_integer_create(integer); \
	} else { \
		linux_exit(-1); \
	} \
}

LONE_LISP_BYTES_READER_PRIMITIVE(u, 8, /* no endianness */)
LONE_LISP_BYTES_READER_PRIMITIVE(s, 8, /* no endianness */)

LONE_LISP_BYTES_WRITER_PRIMITIVE(u, 8, /* no endianness */)
LONE_LISP_BYTES_WRITER_PRIMITIVE(s, 8, /* no endianness */)

LONE_LISP_BYTES_READER_PRIMITIVE(u, 16, /* native endianness */)
LONE_LISP_BYTES_READER_PRIMITIVE(s, 16, /* native endianness */)
LONE_LISP_BYTES_READER_PRIMITIVE(u, 32, /* native endianness */)
LONE_LISP_BYTES_READER_PRIMITIVE(s, 32, /* native endianness */)

LONE_LISP_BYTES_WRITER_PRIMITIVE(u, 16, /* native endianness */)
LONE_LISP_BYTES_WRITER_PRIMITIVE(s, 16, /* native endianness */)
LONE_LISP_BYTES_WRITER_PRIMITIVE(u, 32, /* native endianness */)
LONE_LISP_BYTES_WRITER_PRIMITIVE(s, 32, /* native endianness */)

LONE_LISP_BYTES_READER_PRIMITIVE(u, 16, le)
LONE_LISP_BYTES_READER_PRIMITIVE(s, 16, le)
LONE_LISP_BYTES_READER_PRIMITIVE(u, 32, le)
LONE_LISP_BYTES_READER_PRIMITIVE(s, 32, le)

LONE_LISP_BYTES_WRITER_PRIMITIVE(u, 16, le)
LONE_LISP_BYTES_WRITER_PRIMITIVE(s, 16, le)
LONE_LISP_BYTES_WRITER_PRIMITIVE(u, 32, le)
LONE_LISP_BYTES_WRITER_PRIMITIVE(s, 32, le)

LONE_LISP_BYTES_READER_PRIMITIVE(u, 16, be)
LONE_LISP_BYTES_READER_PRIMITIVE(s, 16, be)
LONE_LISP_BYTES_READER_PRIMITIVE(u, 32, be)
LONE_LISP_BYTES_READER_PRIMITIVE(s, 32, be)

LONE_LISP_BYTES_WRITER_PRIMITIVE(u, 16, be)
LONE_LISP_BYTES_WRITER_PRIMITIVE(s, 16, be)
LONE_LISP_BYTES_WRITER_PRIMITIVE(u, 32, be)
LONE_LISP_BYTES_WRITER_PRIMITIVE(s, 32, be)

#undef LONE_LISP_BYTES_READER_PRIMITIVE
#undef LONE_LISP_BYTES_WRITER_PRIMITIVE
