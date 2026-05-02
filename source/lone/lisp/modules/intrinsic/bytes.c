/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/modules/intrinsic/bytes.h>
#include <lone/lisp/modules/intrinsic/lone.h>

#include <lone/lisp/machine.h>
#include <lone/lisp/machine/stack.h>
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
	struct lone_lisp_value arguments, count;

	switch (step) {
	case 0:

		arguments = lone_lisp_machine_pop_value(lone, machine);

		goto destructure;

	case 1: /* resumed with a replacement count from range-error */

		count = machine->value;

		goto validate_count_range;

	case 2: /* resumed with a replacement count from type-error */

		count = machine->value;

		goto validate_count_type;

	case 3: /* resumed with a replacement arguments list from arity-error */

		arguments = machine->value;

		goto destructure;

	default:
		break;
	}

	linux_exit(-1);

destructure:

	if (lone_lisp_list_destructure(lone, arguments, 1, &count)) {
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				3,
				lone_lisp_intern_c_string(lone, "arity-error"),
				arguments
			);
	}

validate_count_type:

	if (!lone_lisp_is_integer(lone, count)) {
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				2,
				lone_lisp_intern_c_string(lone, "type-error"),
				count
			);
	}

validate_count_range:

	if (lone_lisp_integer_of(count) <= 0) {
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				1,
				lone_lisp_intern_c_string(lone, "range-error"),
				count
			);
	}

	lone_lisp_machine_push_value(lone, machine,
			lone_lisp_bytes_create(lone, lone_lisp_integer_of(count)));
	return 0;
}

LONE_LISP_PRIMITIVE(bytes_is_zero)
{
	struct lone_lisp_value arguments, bytes;
	struct lone_bytes content;

	switch (step) {
	case 0:

		arguments = lone_lisp_machine_pop_value(lone, machine);

		goto destructure;

	case 1: /* resumed with a replacement bytes from type-error */

		bytes = machine->value;

		goto validate_bytes;

	case 2: /* resumed with a replacement arguments list from arity-error */

		arguments = machine->value;

		goto destructure;

	default:
		break;
	}

	linux_exit(-1);

destructure:

	if (lone_lisp_list_destructure(lone, arguments, 1, &bytes)) {
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				2,
				lone_lisp_intern_c_string(lone, "arity-error"),
				arguments
			);
	}

validate_bytes:

	if (!lone_lisp_is_bytes(lone, bytes)) {
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				1,
				lone_lisp_intern_c_string(lone, "type-error"),
				bytes
			);
	}

	content = lone_lisp_bytes_of(lone, &bytes);

	lone_lisp_machine_push_value(lone, machine,
			lone_lisp_boolean_for(lone_bytes_is_zero(content)));

	return 0;
}


static void lone_lisp_bytes_check_read_arguments(struct lone_lisp *lone,
		struct lone_lisp_value bytes, struct lone_lisp_value offset)
{
	if (!lone_lisp_is_bytes(lone, bytes)) {
		/* invalid data source: (reader "text" 0), (reader {} 0) */ linux_exit(-1);
	}

	if (!lone_lisp_is_integer(lone, offset)) {
		/* invalid offset: (reader bytes "invalid"), (reader bytes []) */ linux_exit(-1);
	}

	if (lone_lisp_integer_of(offset) < 0) {
		/* negative offset not supported: (reader bytes -1) */ linux_exit(-1);
	}
}

static void lone_lisp_bytes_check_write_arguments(struct lone_lisp *lone,
		struct lone_lisp_value bytes, struct lone_lisp_value offset, struct lone_lisp_value value)
{
	lone_lisp_bytes_check_read_arguments(lone, bytes, offset);

	if (!lone_lisp_is_integer(lone, value)) {
		/* invalid value: (writer bytes 0 "invalid"), (writer bytes 0 []) */ linux_exit(-1);
	}

	if (lone_lisp_is_frozen(lone, bytes)) {
		/* writes to frozen bytes are forbidden */ linux_exit(-1);
	}
}

#define LONE_LISP_BYTES_READER_PRIMITIVE(sign, bits, endian) \
LONE_LISP_PRIMITIVE(bytes_read_##sign##bits##endian) \
{ \
	struct lone_lisp_value arguments; \
	struct lone_lisp_value bytes; \
	struct lone_lisp_value offset; \
\
	struct lone_optional_##sign##bits integer; \
\
	arguments = lone_lisp_machine_pop_value(lone, machine); \
\
	if (lone_lisp_list_destructure(lone, arguments, 2, &bytes, &offset)) { \
		/* wrong number of arguments */ linux_exit(-1); \
	} \
\
	lone_lisp_bytes_check_read_arguments(lone, bytes, offset); \
\
	integer = lone_bytes_read_##sign##bits##endian(lone_lisp_heap_value_of(lone, bytes)->as.bytes.data, \
			lone_lisp_integer_of(offset)); \
\
	if (integer.present) { \
		lone_lisp_machine_push_value(lone, machine, lone_lisp_integer_create(integer.value)); \
		return 0; \
	} else { \
		linux_exit(-1); \
	} \
}

#define LONE_LISP_BYTES_WRITER_PRIMITIVE(sign, bits, endian) \
LONE_LISP_PRIMITIVE(bytes_write_##sign##bits##endian) \
{ \
	struct lone_lisp_value arguments; \
	struct lone_lisp_value bytes; \
	struct lone_lisp_value offset; \
	struct lone_lisp_value value; \
\
	lone_lisp_integer raw; \
	lone_##sign##bits integer; \
	bool success; \
\
	arguments = lone_lisp_machine_pop_value(lone, machine); \
\
	if (lone_lisp_list_destructure(lone, arguments, 3, &bytes, &offset, &value)) { \
		/* wrong number of arguments */ linux_exit(-1); \
	} \
\
	lone_lisp_bytes_check_write_arguments(lone, bytes, offset, value); \
\
	raw = lone_lisp_integer_of(value); \
	/* Narrowing cast: well-defined for unsigned types (modular reduction). \
	 * For signed types, GCC and Clang both guarantee truncation semantics \
	 * which makes the round-trip equality check below a correct range test. */ \
	integer = (lone_##sign##bits) raw; \
\
	if ((lone_lisp_integer) integer != raw) { \
		/* value out of range for target type */ linux_exit(-1); \
	} \
\
	success = lone_bytes_write_##sign##bits##endian( \
		lone_lisp_heap_value_of(lone, bytes)->as.bytes.data, \
		lone_lisp_integer_of(offset), \
		integer \
	); \
\
	if (success) { \
		lone_lisp_machine_push_value(lone, machine, lone_lisp_integer_create(integer)); \
		return 0; \
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
