/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/types.h>
#include <lone/memory/functions.h>
#include <lone/test.h>
#include <lone/linux.h>

#define JOIN(a, b) a##b

#define PACKED(bits) JOIN(PACKED_, bits)
#define PACKED_8     /* no need to pack struct */
#define PACKED_16    __attribute__((packed))
#define PACKED_32    __attribute__((packed))
#define PACKED_64    __attribute__((packed))

#define LONE_TYPES_TEST_NAME(sign, bits, operation, alignment) \
	"lone/types/" #sign #bits "/" #operation "/" #alignment

#define LONE_TYPES_TEST_FUNCTION(sign, bits, operation, alignment) \
	test_lone_types_lone_##sign##bits##_##operation##_##alignment

#define LONE_TYPES_TEST_READ_ALIGNED(sign, bits, constant)                                         \
static LONE_TEST_FUNCTION(LONE_TYPES_TEST_FUNCTION(sign, bits,                                     \
			read, aligned))                                                            \
{                                                                                                  \
	lone_##sign##bits value = constant;                                                        \
	                                                                                           \
	return                                                                                     \
		(lone_##sign##bits##_read(&value) == constant) ?                                   \
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;                                     \
}

#define LONE_TYPES_TEST_READ_UNALIGNED(sign, bits, constant)                                       \
static LONE_TEST_FUNCTION(LONE_TYPES_TEST_FUNCTION(sign, bits,                                     \
			read, unaligned))                                                          \
{                                                                                                  \
	struct PACKED(bits) {                                                                      \
		unsigned char byte;                                                                \
		lone_##sign##bits value;                                                           \
	} unaligned = { 0, constant };                                                             \
	                                                                                           \
	return                                                                                     \
		(lone_##sign##bits##_read(&unaligned.value) == constant) ?                         \
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;                                     \
}

#define LONE_TYPES_TEST_WRITE_ALIGNED(sign, bits, constant)                                        \
static LONE_TEST_FUNCTION(LONE_TYPES_TEST_FUNCTION(sign, bits,                                     \
			write, aligned))                                                           \
{                                                                                                  \
	lone_##sign##bits value = 0;                                                               \
	                                                                                           \
	lone_##sign##bits##_write(&value, constant);                                               \
	                                                                                           \
	return value == constant ?                                                                 \
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;                                     \
}

#define LONE_TYPES_TEST_WRITE_UNALIGNED(sign, bits, constant)                                      \
static LONE_TEST_FUNCTION(LONE_TYPES_TEST_FUNCTION(sign, bits,                                     \
			write, unaligned))                                                         \
{                                                                                                  \
	struct PACKED(bits) {                                                                      \
		unsigned char byte;                                                                \
		lone_##sign##bits value;                                                           \
	} unaligned = {0};                                                                         \
	                                                                                           \
	lone_##sign##bits##_write(&unaligned.value, constant);                                     \
	                                                                                           \
	return unaligned.value == constant ?                                                       \
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;                                     \
}

LONE_TYPES_TEST_READ_ALIGNED(u, 8,  +214)
LONE_TYPES_TEST_READ_ALIGNED(s, 8,  -42)
LONE_TYPES_TEST_READ_ALIGNED(u, 16, +56535)
LONE_TYPES_TEST_READ_ALIGNED(s, 16, -9001)
LONE_TYPES_TEST_READ_ALIGNED(u, 32, +4294927296)
LONE_TYPES_TEST_READ_ALIGNED(s, 32, -40000)
LONE_TYPES_TEST_READ_ALIGNED(u, 64, +17212176182698430302UL)
LONE_TYPES_TEST_READ_ALIGNED(s, 64, -1234567891011121314)

LONE_TYPES_TEST_READ_UNALIGNED(u, 8,  +214)
LONE_TYPES_TEST_READ_UNALIGNED(s, 8,  -42)
LONE_TYPES_TEST_READ_UNALIGNED(u, 16, +56535)
LONE_TYPES_TEST_READ_UNALIGNED(s, 16, -9001)
LONE_TYPES_TEST_READ_UNALIGNED(u, 32, +4294927296)
LONE_TYPES_TEST_READ_UNALIGNED(s, 32, -40000)
LONE_TYPES_TEST_READ_UNALIGNED(u, 64, +17212176182698430302UL)
LONE_TYPES_TEST_READ_UNALIGNED(s, 64, -1234567891011121314)

LONE_TYPES_TEST_WRITE_ALIGNED(u, 8,  +214)
LONE_TYPES_TEST_WRITE_ALIGNED(s, 8,  -42)
LONE_TYPES_TEST_WRITE_ALIGNED(u, 16, +56535)
LONE_TYPES_TEST_WRITE_ALIGNED(s, 16, -9001)
LONE_TYPES_TEST_WRITE_ALIGNED(u, 32, +4294927296)
LONE_TYPES_TEST_WRITE_ALIGNED(s, 32, -40000)
LONE_TYPES_TEST_WRITE_ALIGNED(u, 64, +17212176182698430302UL)
LONE_TYPES_TEST_WRITE_ALIGNED(s, 64, -1234567891011121314)

LONE_TYPES_TEST_WRITE_UNALIGNED(u, 8,  +214)
LONE_TYPES_TEST_WRITE_UNALIGNED(s, 8,  -42)
LONE_TYPES_TEST_WRITE_UNALIGNED(u, 16, +56535)
LONE_TYPES_TEST_WRITE_UNALIGNED(s, 16, -9001)
LONE_TYPES_TEST_WRITE_UNALIGNED(u, 32, +4294927296)
LONE_TYPES_TEST_WRITE_UNALIGNED(s, 32, -40000)
LONE_TYPES_TEST_WRITE_UNALIGNED(u, 64, +17212176182698430302UL)
LONE_TYPES_TEST_WRITE_UNALIGNED(s, 64, -1234567891011121314)

#undef LONE_TYPES_TEST_READ_ALIGNED
#undef LONE_TYPES_TEST_READ_UNALIGNED
#undef LONE_TYPES_TEST_WRITE_ALIGNED
#undef LONE_TYPES_TEST_WRITE_UNALIGNED

#define LONE_TYPES_BYTES_TEST_NAME(sign, bits, operation, alignment, bounds)                       \
	"lone/types/bytes/" #operation "/" #sign #bits "/" #alignment "/" #bounds

#define LONE_TYPES_BYTES_TEST_FUNCTION(sign, bits, operation, alignment, bounds)                   \
	test_lone_types_lone_bytes_##operation##_##sign##bits##_##alignment##_##bounds

#define LONE_TYPES_BYTES_TEST_READ_ALIGNED_WITHIN_BOUNDS(sign, bits, constant)                     \
static LONE_TEST_FUNCTION(LONE_TYPES_BYTES_TEST_FUNCTION(sign, bits,                               \
			read, aligned, within_bounds))                                             \
{                                                                                                  \
	lone_##sign##bits values[] = { 0, 0, constant, 0, 0 };                                     \
	struct lone_bytes bytes = { sizeof(values), (unsigned char *) &values };                   \
	struct lone_##sign##bits actual = { false, 0 };                                            \
	                                                                                           \
	actual = lone_bytes_read_##sign##bits(bytes, 2 * sizeof(lone_##sign##bits));               \
	                                                                                           \
	return                                                                                     \
		actual.present && actual.value == constant ?                                       \
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;                                     \
}

#define LONE_TYPES_BYTES_TEST_READ_ALIGNED_OUT_OF_BOUNDS(sign, bits, constant)                     \
static LONE_TEST_FUNCTION(LONE_TYPES_BYTES_TEST_FUNCTION(sign, bits,                               \
			read, aligned, out_of_bounds))                                             \
{                                                                                                  \
	lone_##sign##bits values[] = { 0, 0, constant, 0, 0 };                                     \
	struct lone_bytes bytes = { sizeof(values), (unsigned char *) &values };                   \
	struct lone_##sign##bits actual = { false, 0 };                                            \
	                                                                                           \
	actual = lone_bytes_read_##sign##bits(bytes, sizeof(values));                              \
	                                                                                           \
	return                                                                                     \
		!actual.present ?                                                                  \
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;                                     \
}

#define LONE_TYPES_BYTES_TEST_READ_UNALIGNED_WITHIN_BOUNDS(sign, bits, constant)                   \
static LONE_TEST_FUNCTION(LONE_TYPES_BYTES_TEST_FUNCTION(sign, bits,                               \
			read, unaligned, within_bounds))                                           \
{                                                                                                  \
	struct PACKED(bits) {                                                                      \
		unsigned char byte;                                                                \
		lone_##sign##bits value;                                                           \
	} values[] = { {0}, {0}, { 0, constant }, {0}, {0} };                                      \
	struct lone_bytes bytes = { sizeof(values), (unsigned char *) &values };                   \
	struct lone_##sign##bits actual = { false, 0 };                                            \
	                                                                                           \
	actual = lone_bytes_read_##sign##bits(bytes, 2 * sizeof(lone_##sign##bits) + 3);           \
	                                                                                           \
	return                                                                                     \
		actual.present && actual.value == constant ?                                       \
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;                                     \
}

#define LONE_TYPES_BYTES_TEST_READ_UNALIGNED_OUT_OF_BOUNDS(sign, bits, constant)                   \
static LONE_TEST_FUNCTION(LONE_TYPES_BYTES_TEST_FUNCTION(sign, bits,                               \
			read, unaligned, out_of_bounds))                                           \
{                                                                                                  \
	struct PACKED(bits) {                                                                      \
		unsigned char byte;                                                                \
		lone_##sign##bits value;                                                           \
	} values[] = { {0}, {0}, { 0, constant }, {0}, {0} };                                      \
	struct lone_bytes bytes = { sizeof(values), (unsigned char *) &values };                   \
	struct lone_##sign##bits actual = { false, 0 };                                            \
	                                                                                           \
	actual = lone_bytes_read_##sign##bits(bytes, sizeof(values));                              \
	                                                                                           \
	return                                                                                     \
		!actual.present ?                                                                  \
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;                                     \
}

#define LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_WITHIN_BOUNDS(sign, bits, constant)                    \
static LONE_TEST_FUNCTION(LONE_TYPES_BYTES_TEST_FUNCTION(sign, bits,                               \
			write, aligned, within_bounds))                                            \
{                                                                                                  \
	lone_##sign##bits values[] = { 0, 0, constant, 0, 0 };                                     \
	struct lone_bytes bytes = { sizeof(values), (unsigned char *) &values };                   \
	bool written = false;                                                                      \
	                                                                                           \
	written = lone_bytes_write_##sign##bits(                                                   \
		bytes,                                                                             \
		2 * sizeof(lone_##sign##bits),                                                     \
		constant                                                                           \
	);                                                                                         \
	                                                                                           \
	return                                                                                     \
		written && values[2] == constant ?                                                 \
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;                                     \
}

#define LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_OUT_OF_BOUNDS(sign, bits, constant)                    \
static LONE_TEST_FUNCTION(LONE_TYPES_BYTES_TEST_FUNCTION(sign, bits,                               \
			write, aligned, out_of_bounds))                                            \
{                                                                                                  \
	lone_##sign##bits values[] = { 0, 0, constant, 0, 0 };                                     \
	struct lone_bytes bytes = { sizeof(values), (unsigned char *) &values };                   \
	bool written = false;                                                                      \
	                                                                                           \
	written = lone_bytes_write_##sign##bits(bytes, sizeof(values), constant);                  \
	                                                                                           \
	return                                                                                     \
		!written ?                                                                         \
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;                                     \
}

#define LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_WITHIN_BOUNDS(sign, bits, constant)                  \
static LONE_TEST_FUNCTION(LONE_TYPES_BYTES_TEST_FUNCTION(sign, bits,                               \
			write, unaligned, within_bounds))                                          \
{                                                                                                  \
	struct PACKED(bits) {                                                                      \
		unsigned char byte;                                                                \
		lone_##sign##bits value;                                                           \
	} values[] = { {0}, {0}, {0}, {0}, {0} };                                                  \
	struct lone_bytes bytes = { sizeof(values), (unsigned char *) &values };                   \
	bool written = false;                                                                      \
	                                                                                           \
	written = lone_bytes_write_##sign##bits(\
		bytes,                                                                             \
		2 * sizeof(lone_##sign##bits) + 3,                                                 \
		constant                                                                           \
	);                                                                                         \
	                                                                                           \
	return                                                                                     \
		written && values[2].value == constant ?                                           \
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;                                     \
}

#define LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_OUT_OF_BOUNDS(sign, bits, constant)                  \
static LONE_TEST_FUNCTION(LONE_TYPES_BYTES_TEST_FUNCTION(sign, bits,                               \
			write, unaligned, out_of_bounds))                                          \
{                                                                                                  \
	struct PACKED(bits) {                                                                      \
		unsigned char byte;                                                                \
		lone_##sign##bits value;                                                           \
	} values[] = { {0}, {0}, {0}, {0}, {0} };                                                  \
	struct lone_bytes bytes = { sizeof(values), (unsigned char *) &values };                   \
	bool written = false;                                                                      \
	                                                                                           \
	written = lone_bytes_write_##sign##bits(bytes, sizeof(values), constant);                  \
	                                                                                           \
	return                                                                                     \
		!written ?                                                                         \
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;                                     \
}

LONE_TYPES_BYTES_TEST_READ_ALIGNED_WITHIN_BOUNDS(u, 8,  +214)
LONE_TYPES_BYTES_TEST_READ_ALIGNED_WITHIN_BOUNDS(s, 8,  -42)
LONE_TYPES_BYTES_TEST_READ_ALIGNED_WITHIN_BOUNDS(u, 16, +56535)
LONE_TYPES_BYTES_TEST_READ_ALIGNED_WITHIN_BOUNDS(s, 16, -9001)
LONE_TYPES_BYTES_TEST_READ_ALIGNED_WITHIN_BOUNDS(u, 32, +4294927296)
LONE_TYPES_BYTES_TEST_READ_ALIGNED_WITHIN_BOUNDS(s, 32, -40000)
LONE_TYPES_BYTES_TEST_READ_ALIGNED_WITHIN_BOUNDS(u, 64, +17212176182698430302UL)
LONE_TYPES_BYTES_TEST_READ_ALIGNED_WITHIN_BOUNDS(s, 64, -1234567891011121314)

LONE_TYPES_BYTES_TEST_READ_ALIGNED_OUT_OF_BOUNDS(u, 8,  +214)
LONE_TYPES_BYTES_TEST_READ_ALIGNED_OUT_OF_BOUNDS(s, 8,  -42)
LONE_TYPES_BYTES_TEST_READ_ALIGNED_OUT_OF_BOUNDS(u, 16, +56535)
LONE_TYPES_BYTES_TEST_READ_ALIGNED_OUT_OF_BOUNDS(s, 16, -9001)
LONE_TYPES_BYTES_TEST_READ_ALIGNED_OUT_OF_BOUNDS(u, 32, +4294927296)
LONE_TYPES_BYTES_TEST_READ_ALIGNED_OUT_OF_BOUNDS(s, 32, -40000)
LONE_TYPES_BYTES_TEST_READ_ALIGNED_OUT_OF_BOUNDS(u, 64, +17212176182698430302UL)
LONE_TYPES_BYTES_TEST_READ_ALIGNED_OUT_OF_BOUNDS(s, 64, -1234567891011121314)

LONE_TYPES_BYTES_TEST_READ_UNALIGNED_WITHIN_BOUNDS(u, 8,  +214)
LONE_TYPES_BYTES_TEST_READ_UNALIGNED_WITHIN_BOUNDS(s, 8,  -42)
LONE_TYPES_BYTES_TEST_READ_UNALIGNED_WITHIN_BOUNDS(u, 16, +56535)
LONE_TYPES_BYTES_TEST_READ_UNALIGNED_WITHIN_BOUNDS(s, 16, -9001)
LONE_TYPES_BYTES_TEST_READ_UNALIGNED_WITHIN_BOUNDS(u, 32, +4294927296)
LONE_TYPES_BYTES_TEST_READ_UNALIGNED_WITHIN_BOUNDS(s, 32, -40000)
LONE_TYPES_BYTES_TEST_READ_UNALIGNED_WITHIN_BOUNDS(u, 64, +17212176182698430302UL)
LONE_TYPES_BYTES_TEST_READ_UNALIGNED_WITHIN_BOUNDS(s, 64, -1234567891011121314)

LONE_TYPES_BYTES_TEST_READ_UNALIGNED_OUT_OF_BOUNDS(u, 8,  +214)
LONE_TYPES_BYTES_TEST_READ_UNALIGNED_OUT_OF_BOUNDS(s, 8,  -42)
LONE_TYPES_BYTES_TEST_READ_UNALIGNED_OUT_OF_BOUNDS(u, 16, +56535)
LONE_TYPES_BYTES_TEST_READ_UNALIGNED_OUT_OF_BOUNDS(s, 16, -9001)
LONE_TYPES_BYTES_TEST_READ_UNALIGNED_OUT_OF_BOUNDS(u, 32, +4294927296)
LONE_TYPES_BYTES_TEST_READ_UNALIGNED_OUT_OF_BOUNDS(s, 32, -40000)
LONE_TYPES_BYTES_TEST_READ_UNALIGNED_OUT_OF_BOUNDS(u, 64, +17212176182698430302UL)
LONE_TYPES_BYTES_TEST_READ_UNALIGNED_OUT_OF_BOUNDS(s, 64, -1234567891011121314)

LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_WITHIN_BOUNDS(u, 8,  +214)
LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_WITHIN_BOUNDS(s, 8,  -42)
LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_WITHIN_BOUNDS(u, 16, +56535)
LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_WITHIN_BOUNDS(s, 16, -9001)
LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_WITHIN_BOUNDS(u, 32, +4294927296)
LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_WITHIN_BOUNDS(s, 32, -40000)
LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_WITHIN_BOUNDS(u, 64, +17212176182698430302UL)
LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_WITHIN_BOUNDS(s, 64, -1234567891011121314)

LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_OUT_OF_BOUNDS(u, 8,  +214)
LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_OUT_OF_BOUNDS(s, 8,  -42)
LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_OUT_OF_BOUNDS(u, 16, +56535)
LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_OUT_OF_BOUNDS(s, 16, -9001)
LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_OUT_OF_BOUNDS(u, 32, +4294927296)
LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_OUT_OF_BOUNDS(s, 32, -40000)
LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_OUT_OF_BOUNDS(u, 64, +17212176182698430302UL)
LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_OUT_OF_BOUNDS(s, 64, -1234567891011121314)

LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_WITHIN_BOUNDS(u, 8,  +214)
LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_WITHIN_BOUNDS(s, 8,  -42)
LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_WITHIN_BOUNDS(u, 16, +56535)
LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_WITHIN_BOUNDS(s, 16, -9001)
LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_WITHIN_BOUNDS(u, 32, +4294927296)
LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_WITHIN_BOUNDS(s, 32, -40000)
LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_WITHIN_BOUNDS(u, 64, +17212176182698430302UL)
LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_WITHIN_BOUNDS(s, 64, -1234567891011121314)

LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_OUT_OF_BOUNDS(u, 8,  +214)
LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_OUT_OF_BOUNDS(s, 8,  -42)
LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_OUT_OF_BOUNDS(u, 16, +56535)
LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_OUT_OF_BOUNDS(s, 16, -9001)
LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_OUT_OF_BOUNDS(u, 32, +4294927296)
LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_OUT_OF_BOUNDS(s, 32, -40000)
LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_OUT_OF_BOUNDS(u, 64, +17212176182698430302UL)
LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_OUT_OF_BOUNDS(s, 64, -1234567891011121314)

#undef LONE_TYPES_BYTES_TEST_READ_ALIGNED_WITHIN_BOUNDS
#undef LONE_TYPES_BYTES_TEST_READ_ALIGNED_OUT_OF_BOUNDS
#undef LONE_TYPES_BYTES_TEST_READ_UNALIGNED_WITHIN_BOUNDS
#undef LONE_TYPES_BYTES_TEST_READ_UNALIGNED_OUT_OF_BOUNDS
#undef LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_WITHIN_BOUNDS
#undef LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_OUT_OF_BOUNDS
#undef LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_WITHIN_BOUNDS
#undef LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_OUT_OF_BOUNDS

#define LONE_TYPES_ENDIAN_TEST_NAME(sign, bits, operation, endian, alignment)                      \
	"lone/types/" #sign #bits "/" #operation "/" #endian "/" #alignment

#define LONE_TYPES_ENDIAN_TEST_FUNCTION(sign, bits, operation, endian, alignment)                  \
	test_lone_types_lone_##sign##bits##_##operation##_##endian##_##alignment

#define LONE_TYPES_ENDIAN_TEST_READ(sign, bits, endian, alignment, constant, offset, ...)          \
static LONE_TEST_FUNCTION(LONE_TYPES_ENDIAN_TEST_FUNCTION(sign, bits,                              \
			read, endian, alignment))                                                  \
{                                                                                                  \
	unsigned char bytes[] = { __VA_ARGS__ };                                                   \
	                                                                                           \
	return                                                                                     \
		lone_##sign##bits##_read_##endian(bytes + offset) == constant ?                    \
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;                                     \
}

#define LONE_TYPES_ENDIAN_TEST_WRITE(sign, bits, endian, alignment, constant, offset, ...)         \
static LONE_TEST_FUNCTION(LONE_TYPES_ENDIAN_TEST_FUNCTION(sign, bits,                              \
			write, endian, alignment))                                                 \
{                                                                                                  \
	unsigned char actual[sizeof(lone_##sign##bits) + offset] = {0};                            \
	unsigned char expected[] = { __VA_ARGS__ };                                                \
	                                                                                           \
	lone_##sign##bits##_write_##endian(actual + offset, constant);                             \
	                                                                                           \
	return                                                                                     \
		lone_memory_is_equal(actual + offset, expected, sizeof(expected)) ?                \
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;                                     \
}

LONE_TYPES_ENDIAN_TEST_READ(u, 16, le, aligned, 0x0102, 0, 0x02, 0x01)
LONE_TYPES_ENDIAN_TEST_READ(u, 16, be, aligned, 0x0102, 0, 0x01, 0x02)
LONE_TYPES_ENDIAN_TEST_READ(s, 16, le, aligned, 0x0102, 0, 0x02, 0x01)
LONE_TYPES_ENDIAN_TEST_READ(s, 16, be, aligned, 0x0102, 0, 0x01, 0x02)

LONE_TYPES_ENDIAN_TEST_READ(u, 16, le, unaligned, 0x0102, 1, 0x03, 0x02, 0x01)
LONE_TYPES_ENDIAN_TEST_READ(u, 16, be, unaligned, 0x0203, 1, 0x01, 0x02, 0x03)
LONE_TYPES_ENDIAN_TEST_READ(s, 16, le, unaligned, 0x0102, 1, 0x03, 0x02, 0x01)
LONE_TYPES_ENDIAN_TEST_READ(s, 16, be, unaligned, 0x0203, 1, 0x01, 0x02, 0x03)

LONE_TYPES_ENDIAN_TEST_READ(u, 32, le, aligned, 0x01020304, 0, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_ENDIAN_TEST_READ(u, 32, be, aligned, 0x01020304, 0, 0x01, 0x02, 0x03, 0x04)
LONE_TYPES_ENDIAN_TEST_READ(s, 32, le, aligned, 0x01020304, 0, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_ENDIAN_TEST_READ(s, 32, be, aligned, 0x01020304, 0, 0x01, 0x02, 0x03, 0x04)

LONE_TYPES_ENDIAN_TEST_READ(u, 32, le, unaligned, 0x01020304, 1, 0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_ENDIAN_TEST_READ(u, 32, be, unaligned, 0x02030405, 1, 0x01, 0x02, 0x03, 0x04, 0x05)
LONE_TYPES_ENDIAN_TEST_READ(s, 32, le, unaligned, 0x01020304, 1, 0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_ENDIAN_TEST_READ(s, 32, be, unaligned, 0x02030405, 1, 0x01, 0x02, 0x03, 0x04, 0x05)

LONE_TYPES_ENDIAN_TEST_READ(u, 64, le, aligned, 0x0102030405060708, 0, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_ENDIAN_TEST_READ(u, 64, be, aligned, 0x0102030405060708, 0, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08)
LONE_TYPES_ENDIAN_TEST_READ(s, 64, le, aligned, 0x0102030405060708, 0, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_ENDIAN_TEST_READ(s, 64, be, aligned, 0x0102030405060708, 0, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08)

LONE_TYPES_ENDIAN_TEST_READ(u, 64, le, unaligned, 0x0102030405060708, 1, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_ENDIAN_TEST_READ(u, 64, be, unaligned, 0x0203040506070809, 1, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09)
LONE_TYPES_ENDIAN_TEST_READ(s, 64, le, unaligned, 0x0102030405060708, 1, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_ENDIAN_TEST_READ(s, 64, be, unaligned, 0x0203040506070809, 1, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09)

LONE_TYPES_ENDIAN_TEST_WRITE(u, 16, le, aligned, 0x0102, 0, 0x02, 0x01)
LONE_TYPES_ENDIAN_TEST_WRITE(u, 16, be, aligned, 0x0102, 0, 0x01, 0x02)
LONE_TYPES_ENDIAN_TEST_WRITE(s, 16, le, aligned, 0x0102, 0, 0x02, 0x01)
LONE_TYPES_ENDIAN_TEST_WRITE(s, 16, be, aligned, 0x0102, 0, 0x01, 0x02)

LONE_TYPES_ENDIAN_TEST_WRITE(u, 16, le, unaligned, 0x0102, 1, 0x02, 0x01)
LONE_TYPES_ENDIAN_TEST_WRITE(u, 16, be, unaligned, 0x0102, 1, 0x01, 0x02)
LONE_TYPES_ENDIAN_TEST_WRITE(s, 16, le, unaligned, 0x0102, 1, 0x02, 0x01)
LONE_TYPES_ENDIAN_TEST_WRITE(s, 16, be, unaligned, 0x0102, 1, 0x01, 0x02)

LONE_TYPES_ENDIAN_TEST_WRITE(u, 32, le, aligned, 0x01020304, 0, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_ENDIAN_TEST_WRITE(u, 32, be, aligned, 0x01020304, 0, 0x01, 0x02, 0x03, 0x04)
LONE_TYPES_ENDIAN_TEST_WRITE(s, 32, le, aligned, 0x01020304, 0, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_ENDIAN_TEST_WRITE(s, 32, be, aligned, 0x01020304, 0, 0x01, 0x02, 0x03, 0x04)

LONE_TYPES_ENDIAN_TEST_WRITE(u, 32, le, unaligned, 0x01020304, 1, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_ENDIAN_TEST_WRITE(u, 32, be, unaligned, 0x01020304, 1, 0x01, 0x02, 0x03, 0x04)
LONE_TYPES_ENDIAN_TEST_WRITE(s, 32, le, unaligned, 0x01020304, 1, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_ENDIAN_TEST_WRITE(s, 32, be, unaligned, 0x01020304, 1, 0x01, 0x02, 0x03, 0x04)

LONE_TYPES_ENDIAN_TEST_WRITE(u, 64, le, aligned, 0x0102030405060708, 0, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_ENDIAN_TEST_WRITE(u, 64, be, aligned, 0x0102030405060708, 0, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08)
LONE_TYPES_ENDIAN_TEST_WRITE(s, 64, le, aligned, 0x0102030405060708, 0, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_ENDIAN_TEST_WRITE(s, 64, be, aligned, 0x0102030405060708, 0, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08)

LONE_TYPES_ENDIAN_TEST_WRITE(u, 64, le, unaligned, 0x0102030405060708, 1, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_ENDIAN_TEST_WRITE(u, 64, be, unaligned, 0x0102030405060708, 1, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08)
LONE_TYPES_ENDIAN_TEST_WRITE(s, 64, le, unaligned, 0x0102030405060708, 1, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_ENDIAN_TEST_WRITE(s, 64, be, unaligned, 0x0102030405060708, 1, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08)

#undef LONE_TYPES_ENDIAN_TEST_READ_ALIGNED

static void test_finished(struct lone_test_case *test, void *context)
{
	struct lone_bytes result;

	switch (test->result) {
	case LONE_TEST_RESULT_PASS:
		result = (struct lone_bytes) LONE_BYTES_FROM_LITERAL("PASS");
		break;
	case LONE_TEST_RESULT_FAIL:
		result = (struct lone_bytes) LONE_BYTES_FROM_LITERAL("FAIL");
		break;
	case LONE_TEST_RESULT_SKIP:
		result = (struct lone_bytes) LONE_BYTES_FROM_LITERAL("SKIP");
		break;
	case LONE_TEST_RESULT_PENDING:    __attribute__((fallthrough));
	default:
		linux_exit(-1);
	}

	linux_write(1, result.pointer, result.count);
	linux_write(1, " ", 1);
	linux_write(1, test->name.pointer, test->name.count);
	linux_write(1, "\n", 1);
}

long lone(int argc, char **argv, char **envp, struct lone_auxiliary_vector *auxv)
{

	static struct lone_test_case cases[] = {

#define LONE_TYPES_TEST_CASE_4(sign, bits, operation, alignment)                                   \
	LONE_TEST_CASE(LONE_TYPES_TEST_NAME(sign, bits, operation, alignment),                     \
		LONE_TYPES_TEST_FUNCTION(sign, bits, operation, alignment)),

#define LONE_TYPES_TEST_CASE_3(sign, bits, operation)                                              \
	LONE_TYPES_TEST_CASE_4(sign, bits, operation, aligned)                                     \
	LONE_TYPES_TEST_CASE_4(sign, bits, operation, unaligned)

#define LONE_TYPES_TEST_CASE_2(sign, bits)                                                         \
	LONE_TYPES_TEST_CASE_3(sign, bits, read)                                                   \
	LONE_TYPES_TEST_CASE_3(sign, bits, write)

#define LONE_TYPES_TEST_CASE_1(sign)                                                               \
	LONE_TYPES_TEST_CASE_2(sign, 8)                                                            \
	LONE_TYPES_TEST_CASE_2(sign, 16)                                                           \
	LONE_TYPES_TEST_CASE_2(sign, 32)                                                           \
	LONE_TYPES_TEST_CASE_2(sign, 64)

#define LONE_TYPES_TEST_CASES()                                                                    \
	LONE_TYPES_TEST_CASE_1(u)                                                                  \
	LONE_TYPES_TEST_CASE_1(s)

		LONE_TYPES_TEST_CASES()

#undef LONE_TYPES_TEST_CASES
#undef LONE_TYPES_TEST_CASE_1
#undef LONE_TYPES_TEST_CASE_2
#undef LONE_TYPES_TEST_CASE_3
#undef LONE_TYPES_TEST_CASE_4

#define LONE_TYPES_BYTES_TEST_CASE_5(sign, bits, operation, alignment, bounds)                     \
	LONE_TEST_CASE(LONE_TYPES_BYTES_TEST_NAME(sign, bits, operation, alignment, bounds),       \
		LONE_TYPES_BYTES_TEST_FUNCTION(sign, bits, operation, alignment, bounds)),

#define LONE_TYPES_BYTES_TEST_CASE_4(sign, bits, operation, alignment)                             \
	LONE_TYPES_BYTES_TEST_CASE_5(sign, bits, operation, alignment, within_bounds)              \
	LONE_TYPES_BYTES_TEST_CASE_5(sign, bits, operation, alignment, out_of_bounds)

#define LONE_TYPES_BYTES_TEST_CASE_3(sign, bits, operation)                                        \
	LONE_TYPES_BYTES_TEST_CASE_4(sign, bits, operation, aligned)                               \
	LONE_TYPES_BYTES_TEST_CASE_4(sign, bits, operation, unaligned)

#define LONE_TYPES_BYTES_TEST_CASE_2(sign, bits)                                                   \
	LONE_TYPES_BYTES_TEST_CASE_3(sign, bits, read)                                             \
	LONE_TYPES_BYTES_TEST_CASE_3(sign, bits, write)

#define LONE_TYPES_BYTES_TEST_CASE_1(sign)                                                         \
	LONE_TYPES_BYTES_TEST_CASE_2(sign, 8)                                                      \
	LONE_TYPES_BYTES_TEST_CASE_2(sign, 16)                                                     \
	LONE_TYPES_BYTES_TEST_CASE_2(sign, 32)                                                     \
	LONE_TYPES_BYTES_TEST_CASE_2(sign, 64)

#define LONE_TYPES_BYTES_TEST_CASES(type)                                                          \
	LONE_TYPES_BYTES_TEST_CASE_1(u)                                                            \
	LONE_TYPES_BYTES_TEST_CASE_1(s)

		LONE_TYPES_BYTES_TEST_CASES()

#undef LONE_TYPES_BYTES_TEST_CASES
#undef LONE_TYPES_BYTES_TEST_CASE_1
#undef LONE_TYPES_BYTES_TEST_CASE_2
#undef LONE_TYPES_BYTES_TEST_CASE_3
#undef LONE_TYPES_BYTES_TEST_CASE_4
#undef LONE_TYPES_BYTES_TEST_CASE_5

#define LONE_TYPES_ENDIAN_TEST_CASE_5(sign, bits, operation, endian, alignment)                    \
	LONE_TEST_CASE(LONE_TYPES_ENDIAN_TEST_NAME(sign, bits, operation, endian, alignment),      \
		LONE_TYPES_ENDIAN_TEST_FUNCTION(sign, bits, operation, endian, alignment)),

#define LONE_TYPES_ENDIAN_TEST_CASE_4(sign, bits, operation, endian)                               \
	LONE_TYPES_ENDIAN_TEST_CASE_5(sign, bits, operation, endian, aligned)                      \
	LONE_TYPES_ENDIAN_TEST_CASE_5(sign, bits, operation, endian, unaligned)

#define LONE_TYPES_ENDIAN_TEST_CASE_3(sign, bits, operation)                                       \
	LONE_TYPES_ENDIAN_TEST_CASE_4(sign, bits, operation, le)                                   \
	LONE_TYPES_ENDIAN_TEST_CASE_4(sign, bits, operation, be)

#define LONE_TYPES_ENDIAN_TEST_CASE_2(sign, bits)                                                  \
	LONE_TYPES_ENDIAN_TEST_CASE_3(sign, bits, read)                                            \
	LONE_TYPES_ENDIAN_TEST_CASE_3(sign, bits, write)

#define LONE_TYPES_ENDIAN_TEST_CASE_1(sign)                                                        \
	LONE_TYPES_ENDIAN_TEST_CASE_2(sign, 16)                                                    \
	LONE_TYPES_ENDIAN_TEST_CASE_2(sign, 32)                                                    \
	LONE_TYPES_ENDIAN_TEST_CASE_2(sign, 64)

#define LONE_TYPES_ENDIAN_TEST_CASES()                                                             \
	LONE_TYPES_ENDIAN_TEST_CASE_1(u)                                                           \
	LONE_TYPES_ENDIAN_TEST_CASE_1(s)                                                           \

		LONE_TYPES_ENDIAN_TEST_CASES()                                                     \

#undef LONE_TYPES_ENDIAN_TEST_CASES
#undef LONE_TYPES_ENDIAN_TEST_CASE_1
#undef LONE_TYPES_ENDIAN_TEST_CASE_2
#undef LONE_TYPES_ENDIAN_TEST_CASE_3
#undef LONE_TYPES_ENDIAN_TEST_CASE_4
#undef LONE_TYPES_ENDIAN_TEST_CASE_5

		LONE_TEST_CASE_NULL(),
	};

	struct lone_test_suite suite = LONE_TEST_SUITE(cases);
	enum lone_test_result result;

	suite.events.on.test.finished = test_finished;

	result = lone_test_suite_run(&suite);

	switch (result) {
	case LONE_TEST_RESULT_PASS:
		return 0;
	case LONE_TEST_RESULT_FAIL:
		return 1;
	case LONE_TEST_RESULT_SKIP:
		return 2;
	default:
		return -1;
	}
}

#undef LONE_TYPES_ENDIAN_TEST_NAME
#undef LONE_TYPES_ENDIAN_TEST_FUNCTION

#undef LONE_TYPES_BYTES_TEST_NAME
#undef LONE_TYPES_BYTES_TEST_FUNCTION

#undef LONE_TYPES_TEST_NAME
#undef LONE_TYPES_TEST_FUNCTION

#include <lone/architecture/linux/entry_point.c>
