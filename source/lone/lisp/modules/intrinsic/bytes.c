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
	LONE_LISP_EXPORT_BYTES_READER_PRIMITIVE(u, 64, /* native endianness */);
	LONE_LISP_EXPORT_BYTES_READER_PRIMITIVE(s, 64, /* native endianness */);

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

#undef LONE_LISP_EXPORT_BYTES_WRITER_PRIMITIVE
#undef LONE_LISP_EXPORT_BYTES_READER_PRIMITIVE
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

#define LONE_LISP_BYTES_INTEGER_OVERFLOW_GUARD_8(sign)
#define LONE_LISP_BYTES_INTEGER_OVERFLOW_GUARD_16(sign)
#define LONE_LISP_BYTES_INTEGER_OVERFLOW_GUARD_32(sign)

#define LONE_LISP_BYTES_INTEGER_OVERFLOW_GUARD_64(sign)                                            \
	if (LONE_LISP_BYTES_INTEGER_OVERFLOW_CHECK_64_##sign) {                                    \
		lone_lisp_machine_push_value(lone, machine, bytes);                                \
		return                                                                             \
			lone_lisp_signal_emit(                                                     \
				lone,                                                              \
				machine,                                                           \
				4,                                                                 \
				lone_lisp_intern_c_string(lone, "integer-overflow"),               \
				offset                                                             \
			);                                                                         \
	}

#define LONE_LISP_BYTES_INTEGER_OVERFLOW_CHECK_64_u \
	(actual > (lone_u64) LONE_LISP_INTEGER_MAX)

#define LONE_LISP_BYTES_INTEGER_OVERFLOW_CHECK_64_s \
	(actual < LONE_LISP_INTEGER_MIN || actual > LONE_LISP_INTEGER_MAX)

#define LONE_LISP_BYTES_READER_PRIMITIVE(sign, bits, endian)                                       \
LONE_LISP_PRIMITIVE(bytes_read_##sign##bits##endian)                                               \
{                                                                                                  \
	struct lone_lisp_value arguments, bytes, offset;                                           \
	struct lone_optional_##sign##bits read;                                                    \
	lone_##sign##bits actual;                                                                  \
                                                                                                   \
	switch (step) {                                                                            \
	case 0:                                                                                    \
                                                                                                   \
		arguments = lone_lisp_machine_pop_value(lone, machine);                            \
                                                                                                   \
		goto destructure;                                                                  \
                                                                                                   \
	case 1: /* resumed with a replacement arguments list from arity-error */                   \
                                                                                                   \
		arguments = machine->value;                                                        \
                                                                                                   \
		goto destructure;                                                                  \
                                                                                                   \
	case 2: /* resumed with a replacement bytes from type-error */                             \
                                                                                                   \
		bytes = machine->value;                                                            \
		offset = lone_lisp_machine_pop_value(lone, machine);                               \
                                                                                                   \
		goto validate_bytes;                                                               \
                                                                                                   \
	case 3: /* resumed with a replacement offset from type-error */                            \
                                                                                                   \
		offset = machine->value;                                                           \
		bytes = lone_lisp_machine_pop_value(lone, machine);                                \
                                                                                                   \
		goto validate_offset_type;                                                         \
                                                                                                   \
	case 4: /* resumed with a replacement offset from index-error or integer-overflow */       \
                                                                                                   \
		offset = machine->value;                                                           \
		bytes = lone_lisp_machine_pop_value(lone, machine);                                \
                                                                                                   \
		goto validate_offset_range;                                                        \
                                                                                                   \
	default:                                                                                   \
		break;                                                                             \
	}                                                                                          \
                                                                                                   \
	linux_exit(-1);                                                                            \
                                                                                                   \
destructure:                                                                                       \
                                                                                                   \
	if (lone_lisp_list_destructure(lone, arguments, 2, &bytes, &offset)) {                     \
		return                                                                             \
			lone_lisp_signal_emit(                                                     \
				lone,                                                              \
				machine,                                                           \
				1,                                                                 \
				lone_lisp_intern_c_string(lone, "arity-error"),                    \
				arguments                                                          \
			);                                                                         \
	}                                                                                          \
                                                                                                   \
validate_bytes:                                                                                    \
                                                                                                   \
	if (!lone_lisp_is_bytes(lone, bytes)) {                                                    \
		lone_lisp_machine_push_value(lone, machine, offset);                               \
		return                                                                             \
			lone_lisp_signal_emit(                                                     \
				lone,                                                              \
				machine,                                                           \
				2,                                                                 \
				lone_lisp_intern_c_string(lone, "type-error"),                     \
				bytes                                                              \
			);                                                                         \
	}                                                                                          \
                                                                                                   \
validate_offset_type:                                                                              \
                                                                                                   \
	if (!lone_lisp_is_integer(lone, offset)) {                                                 \
		lone_lisp_machine_push_value(lone, machine, bytes);                                \
		return                                                                             \
			lone_lisp_signal_emit(                                                     \
				lone,                                                              \
				machine,                                                           \
				3,                                                                 \
				lone_lisp_intern_c_string(lone, "type-error"),                     \
				offset                                                             \
			);                                                                         \
	}                                                                                          \
                                                                                                   \
validate_offset_range:                                                                             \
                                                                                                   \
	if (lone_lisp_integer_of(offset) < 0) {                                                    \
		lone_lisp_machine_push_value(lone, machine, bytes);                                \
		return                                                                             \
			lone_lisp_signal_emit(                                                     \
				lone,                                                              \
				machine,                                                           \
				4,                                                                 \
				lone_lisp_intern_c_string(lone, "index-error"),                    \
				offset                                                             \
			);                                                                         \
	}                                                                                          \
                                                                                                   \
	read = lone_bytes_read_##sign##bits##endian(                                               \
		lone_lisp_heap_value_of(lone, bytes)->as.bytes.data,                               \
		lone_lisp_integer_of(offset)                                                       \
	);                                                                                         \
                                                                                                   \
	if (!read.present) {                                                                       \
		lone_lisp_machine_push_value(lone, machine, bytes);                                \
		return                                                                             \
			lone_lisp_signal_emit(                                                     \
				lone,                                                              \
				machine,                                                           \
				4,                                                                 \
				lone_lisp_intern_c_string(lone, "index-error"),                    \
				offset                                                             \
			);                                                                         \
	}                                                                                          \
                                                                                                   \
	actual = read.value;                                                                       \
                                                                                                   \
	LONE_LISP_BYTES_INTEGER_OVERFLOW_GUARD_##bits(sign)                                        \
                                                                                                   \
	lone_lisp_machine_push_value(lone, machine, lone_lisp_integer_create(actual));             \
	return 0;                                                                                  \
}

#define LONE_LISP_BYTES_WRITER_PRIMITIVE(sign, bits, endian)                                       \
LONE_LISP_PRIMITIVE(bytes_write_##sign##bits##endian)                                              \
{                                                                                                  \
	struct lone_lisp_value arguments, bytes, offset, value;                                    \
	lone_lisp_integer raw;                                                                     \
	lone_##sign##bits integer;                                                                 \
	bool success;                                                                              \
                                                                                                   \
	switch (step) {                                                                            \
	case 0:                                                                                    \
                                                                                                   \
		arguments = lone_lisp_machine_pop_value(lone, machine);                            \
                                                                                                   \
		goto destructure;                                                                  \
                                                                                                   \
	case 1: /* resumed with a replacement arguments list from arity-error */                   \
                                                                                                   \
		arguments = machine->value;                                                        \
                                                                                                   \
		goto destructure;                                                                  \
                                                                                                   \
	case 2: /* resumed with a replacement bytes from type-error or frozen-error */             \
                                                                                                   \
		bytes = machine->value;                                                            \
		value = lone_lisp_machine_pop_value(lone, machine);                                \
		offset = lone_lisp_machine_pop_value(lone, machine);                               \
                                                                                                   \
		goto validate_bytes;                                                               \
                                                                                                   \
	case 3: /* resumed with a replacement offset from type-error */                            \
                                                                                                   \
		offset = machine->value;                                                           \
		value = lone_lisp_machine_pop_value(lone, machine);                                \
		bytes = lone_lisp_machine_pop_value(lone, machine);                                \
                                                                                                   \
		goto validate_offset_type;                                                         \
                                                                                                   \
	case 4: /* resumed with a replacement value from type-error */                             \
                                                                                                   \
		value = machine->value;                                                            \
		offset = lone_lisp_machine_pop_value(lone, machine);                               \
		bytes = lone_lisp_machine_pop_value(lone, machine);                                \
                                                                                                   \
		goto validate_value_type;                                                          \
                                                                                                   \
	case 5: /* resumed with a replacement offset from index-error */                           \
                                                                                                   \
		offset = machine->value;                                                           \
		value = lone_lisp_machine_pop_value(lone, machine);                                \
		bytes = lone_lisp_machine_pop_value(lone, machine);                                \
                                                                                                   \
		goto validate_offset_range;                                                        \
                                                                                                   \
	case 6: /* resumed with a replacement value from range-error */                            \
                                                                                                   \
		value = machine->value;                                                            \
		offset = lone_lisp_machine_pop_value(lone, machine);                               \
		bytes = lone_lisp_machine_pop_value(lone, machine);                                \
                                                                                                   \
		goto validate_value_range;                                                         \
                                                                                                   \
	default:                                                                                   \
		break;                                                                             \
	}                                                                                          \
                                                                                                   \
	linux_exit(-1);                                                                            \
                                                                                                   \
destructure:                                                                                       \
                                                                                                   \
	if (lone_lisp_list_destructure(lone, arguments, 3, &bytes, &offset, &value)) {             \
		return                                                                             \
			lone_lisp_signal_emit(                                                     \
				lone,                                                              \
				machine,                                                           \
				1,                                                                 \
				lone_lisp_intern_c_string(lone, "arity-error"),                    \
				arguments                                                          \
			);                                                                         \
	}                                                                                          \
                                                                                                   \
validate_bytes:                                                                                    \
                                                                                                   \
	if (!lone_lisp_is_bytes(lone, bytes)) {                                                    \
		lone_lisp_machine_push_value(lone, machine, offset);                               \
		lone_lisp_machine_push_value(lone, machine, value);                                \
		return                                                                             \
			lone_lisp_signal_emit(                                                     \
				lone,                                                              \
				machine,                                                           \
				2,                                                                 \
				lone_lisp_intern_c_string(lone, "type-error"),                     \
				bytes                                                              \
			);                                                                         \
	}                                                                                          \
                                                                                                   \
validate_offset_type:                                                                              \
                                                                                                   \
	if (!lone_lisp_is_integer(lone, offset)) {                                                 \
		lone_lisp_machine_push_value(lone, machine, bytes);                                \
		lone_lisp_machine_push_value(lone, machine, value);                                \
		return                                                                             \
			lone_lisp_signal_emit(                                                     \
				lone,                                                              \
				machine,                                                           \
				3,                                                                 \
				lone_lisp_intern_c_string(lone, "type-error"),                     \
				offset                                                             \
			);                                                                         \
	}                                                                                          \
                                                                                                   \
validate_value_type:                                                                               \
                                                                                                   \
	if (!lone_lisp_is_integer(lone, value)) {                                                  \
		lone_lisp_machine_push_value(lone, machine, bytes);                                \
		lone_lisp_machine_push_value(lone, machine, offset);                               \
		return                                                                             \
			lone_lisp_signal_emit(                                                     \
				lone,                                                              \
				machine,                                                           \
				4,                                                                 \
				lone_lisp_intern_c_string(lone, "type-error"),                     \
				value                                                              \
			);                                                                         \
	}                                                                                          \
                                                                                                   \
validate_offset_range:                                                                             \
                                                                                                   \
	if (lone_lisp_integer_of(offset) < 0) {                                                    \
		lone_lisp_machine_push_value(lone, machine, bytes);                                \
		lone_lisp_machine_push_value(lone, machine, value);                                \
		return                                                                             \
			lone_lisp_signal_emit(                                                     \
				lone,                                                              \
				machine,                                                           \
				5,                                                                 \
				lone_lisp_intern_c_string(lone, "index-error"),                    \
				offset                                                             \
			);                                                                         \
	}                                                                                          \
                                                                                                   \
validate_value_range:                                                                              \
                                                                                                   \
	raw = lone_lisp_integer_of(value);                                                         \
	/* Narrowing cast: well-defined for unsigned types (modular reduction).                    \
	 * For signed types, GCC and Clang both guarantee truncation semantics                     \
	 * which makes the round-trip equality check below a correct range test. */                \
	integer = (lone_##sign##bits) raw;                                                         \
                                                                                                   \
	if ((lone_lisp_integer) integer != raw) {                                                  \
		lone_lisp_machine_push_value(lone, machine, bytes);                                \
		lone_lisp_machine_push_value(lone, machine, offset);                               \
		return                                                                             \
			lone_lisp_signal_emit(                                                     \
				lone,                                                              \
				machine,                                                           \
				6,                                                                 \
				lone_lisp_intern_c_string(lone, "range-error"),                    \
				value                                                              \
			);                                                                         \
	}                                                                                          \
                                                                                                   \
	if (lone_lisp_is_frozen(lone, bytes)) {                                                    \
		lone_lisp_machine_push_value(lone, machine, offset);                               \
		lone_lisp_machine_push_value(lone, machine, value);                                \
		return                                                                             \
			lone_lisp_signal_emit(                                                     \
				lone,                                                              \
				machine,                                                           \
				2,                                                                 \
				lone_lisp_intern_c_string(lone, "frozen-error"),                   \
				bytes                                                              \
			);                                                                         \
	}                                                                                          \
                                                                                                   \
	success = lone_bytes_write_##sign##bits##endian(                                           \
		lone_lisp_heap_value_of(lone, bytes)->as.bytes.data,                               \
		lone_lisp_integer_of(offset),                                                      \
		integer                                                                            \
	);                                                                                         \
                                                                                                   \
	if (!success) {                                                                            \
		lone_lisp_machine_push_value(lone, machine, bytes);                                \
		lone_lisp_machine_push_value(lone, machine, value);                                \
		return                                                                             \
			lone_lisp_signal_emit(                                                     \
				lone,                                                              \
				machine,                                                           \
				5,                                                                 \
				lone_lisp_intern_c_string(lone, "index-error"),                    \
				offset                                                             \
			);                                                                         \
	}                                                                                          \
                                                                                                   \
	lone_lisp_machine_push_value(lone, machine, lone_lisp_integer_create(integer));            \
	return 0;                                                                                  \
}

LONE_LISP_BYTES_READER_PRIMITIVE(u, 8, /* no endianness */)
LONE_LISP_BYTES_READER_PRIMITIVE(s, 8, /* no endianness */)

LONE_LISP_BYTES_WRITER_PRIMITIVE(u, 8, /* no endianness */)
LONE_LISP_BYTES_WRITER_PRIMITIVE(s, 8, /* no endianness */)

LONE_LISP_BYTES_READER_PRIMITIVE(u, 16, /* native endianness */)
LONE_LISP_BYTES_READER_PRIMITIVE(s, 16, /* native endianness */)
LONE_LISP_BYTES_READER_PRIMITIVE(u, 32, /* native endianness */)
LONE_LISP_BYTES_READER_PRIMITIVE(s, 32, /* native endianness */)
LONE_LISP_BYTES_READER_PRIMITIVE(u, 64, /* native endianness */)
LONE_LISP_BYTES_READER_PRIMITIVE(s, 64, /* native endianness */)

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

#undef LONE_LISP_BYTES_WRITER_PRIMITIVE
#undef LONE_LISP_BYTES_READER_PRIMITIVE

#undef LONE_LISP_BYTES_INTEGER_OVERFLOW_CHECK_64_s
#undef LONE_LISP_BYTES_INTEGER_OVERFLOW_CHECK_64_u

#undef LONE_LISP_BYTES_INTEGER_OVERFLOW_GUARD_64
#undef LONE_LISP_BYTES_INTEGER_OVERFLOW_GUARD_32
#undef LONE_LISP_BYTES_INTEGER_OVERFLOW_GUARD_16
#undef LONE_LISP_BYTES_INTEGER_OVERFLOW_GUARD_8
