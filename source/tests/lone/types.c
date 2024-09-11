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
	lone_test_assert_##sign##bits##_equal(suite, test,                                         \
			lone_##sign##bits##_read(&value), constant);                               \
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
	lone_test_assert_##sign##bits##_equal(suite, test,                                         \
			lone_##sign##bits##_read(&unaligned.value), constant);                     \
}

#define LONE_TYPES_TEST_WRITE_ALIGNED(sign, bits, constant)                                        \
static LONE_TEST_FUNCTION(LONE_TYPES_TEST_FUNCTION(sign, bits,                                     \
			write, aligned))                                                           \
{                                                                                                  \
	lone_##sign##bits value = 0;                                                               \
	                                                                                           \
	lone_##sign##bits##_write(&value, constant);                                               \
	                                                                                           \
	lone_test_assert_##sign##bits##_equal(suite, test, value, constant);                       \
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
	lone_test_assert_##sign##bits##_equal(suite, test, unaligned.value, constant);             \
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
	struct lone_optional_##sign##bits actual = { false, 0 };                                   \
	                                                                                           \
	actual = lone_bytes_read_##sign##bits(bytes, 2 * sizeof(lone_##sign##bits));               \
	                                                                                           \
	lone_test_assert_true(suite, test, actual.present);                                        \
	lone_test_assert_##sign##bits##_equal(suite, test, actual.value, constant);                \
}

#define LONE_TYPES_BYTES_TEST_READ_ALIGNED_OUT_OF_BOUNDS(sign, bits, constant)                     \
static LONE_TEST_FUNCTION(LONE_TYPES_BYTES_TEST_FUNCTION(sign, bits,                               \
			read, aligned, out_of_bounds))                                             \
{                                                                                                  \
	lone_##sign##bits values[] = { 0, 0, constant, 0, 0 };                                     \
	struct lone_bytes bytes = { sizeof(values), (unsigned char *) &values };                   \
	struct lone_optional_##sign##bits actual = { false, 0 };                                   \
	                                                                                           \
	actual = lone_bytes_read_##sign##bits(bytes, sizeof(values));                              \
	                                                                                           \
	lone_test_assert_false(suite, test, actual.present);                                       \
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
	struct lone_optional_##sign##bits actual = { false, 0 };                                   \
	                                                                                           \
	actual = lone_bytes_read_##sign##bits(bytes, 2 * sizeof(lone_##sign##bits) + 3);           \
	                                                                                           \
	lone_test_assert_true(suite, test, actual.present);                                        \
	lone_test_assert_##sign##bits##_equal(suite, test, actual.value, constant);                \
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
	struct lone_optional_##sign##bits actual = { false, 0 };                                   \
	                                                                                           \
	actual = lone_bytes_read_##sign##bits(bytes, sizeof(values));                              \
	                                                                                           \
	lone_test_assert_false(suite, test, actual.present);                                       \
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
	lone_test_assert_true(suite, test, written);                                               \
	lone_test_assert_##sign##bits##_equal(suite, test, values[2], constant);                   \
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
	lone_test_assert_false(suite, test, written);                                              \
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
	lone_test_assert_true(suite, test, written);                                               \
	lone_test_assert_##sign##bits##_equal(suite, test, values[2].value, constant);             \
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
	lone_test_assert_false(suite, test, written);                                              \
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
	"lone/types/" #sign #bits #endian "/" #operation "/" #alignment

#define LONE_TYPES_ENDIAN_TEST_FUNCTION(sign, bits, operation, endian, alignment)                  \
	test_lone_types_lone_##sign##bits##endian##_##operation##_##alignment

#define LONE_TYPES_ENDIAN_TEST_READ(sign, bits, endian, alignment, constant, offset, ...)          \
static LONE_TEST_FUNCTION(LONE_TYPES_ENDIAN_TEST_FUNCTION(sign, bits,                              \
			read, endian, alignment))                                                  \
{                                                                                                  \
	unsigned char bytes[] = { __VA_ARGS__ };                                                   \
	                                                                                           \
	lone_test_assert_##sign##bits##_equal(suite, test,                                         \
			lone_##sign##bits##endian##_read(bytes + offset), constant);               \
}

#define LONE_TYPES_ENDIAN_TEST_WRITE(sign, bits, endian, alignment, constant, offset, ...)         \
static LONE_TEST_FUNCTION(LONE_TYPES_ENDIAN_TEST_FUNCTION(sign, bits,                              \
			write, endian, alignment))                                                 \
{                                                                                                  \
	unsigned char actual[sizeof(lone_##sign##bits) + offset] = {0};                            \
	unsigned char expected[] = { __VA_ARGS__ };                                                \
	                                                                                           \
	lone_##sign##bits##endian##_write(actual + offset, constant);                              \
	                                                                                           \
	lone_test_assert_true(suite, test,                                                         \
			lone_memory_is_equal(actual + offset, expected, sizeof(expected)));        \
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

#undef LONE_TYPES_ENDIAN_TEST_WRITE
#undef LONE_TYPES_ENDIAN_TEST_READ

#define LONE_TYPES_BYTES_ENDIAN_TEST_NAME(sign, bits, operation, endian, alignment, bounds)        \
	"lone/types/bytes/" #operation "/" #sign #bits #endian "/" #alignment "/" #bounds

#define LONE_TYPES_BYTES_ENDIAN_TEST_FUNCTION(sign, bits, operation, endian, alignment, bounds)    \
	test_lone_types_lone_bytes_##operation##_##sign##bits##endian##_##alignment##_##bounds

#define LONE_TYPES_BYTES_ENDIAN_TEST_READ_WITHIN_BOUNDS(sign, bits, endian, alignment,             \
		constant, offset, ...)                                                             \
static LONE_TEST_FUNCTION(LONE_TYPES_BYTES_ENDIAN_TEST_FUNCTION(sign, bits,                        \
			read, endian, alignment, within_bounds))                                   \
{                                                                                                  \
	unsigned char data[] = { __VA_ARGS__ };                                                    \
	struct lone_bytes bytes = { .count = sizeof(data), .pointer = data };                      \
	struct lone_optional_##sign##bits actual;                                                  \
	                                                                                           \
	actual = lone_bytes_read_##sign##bits##endian(bytes, offset);                              \
	                                                                                           \
	lone_test_assert_true(suite, test, actual.present);                                        \
	lone_test_assert_##sign##bits##_equal(suite, test, actual.value, constant);                \
}

#define LONE_TYPES_BYTES_ENDIAN_TEST_READ_OUT_OF_BOUNDS(sign, bits, endian, alignment,             \
		constant, offset, ...)                                                             \
static LONE_TEST_FUNCTION(LONE_TYPES_BYTES_ENDIAN_TEST_FUNCTION(sign, bits,                        \
			read, endian, alignment, out_of_bounds))                                   \
{                                                                                                  \
	unsigned char data[] = { __VA_ARGS__ };                                                    \
	struct lone_bytes bytes = { .count = sizeof(data), .pointer = data };                      \
	struct lone_optional_##sign##bits actual;                                                  \
	                                                                                           \
	actual = lone_bytes_read_##sign##bits##endian(bytes, sizeof(data));                        \
	                                                                                           \
	lone_test_assert_false(suite, test, actual.present);                                       \
}

#define LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_WITHIN_BOUNDS(sign, bits, endian, alignment,            \
		constant, offset, ...)                                                             \
static LONE_TEST_FUNCTION(LONE_TYPES_BYTES_ENDIAN_TEST_FUNCTION(sign, bits,                        \
			write, endian, alignment, within_bounds))                                  \
{                                                                                                  \
	unsigned char actual[sizeof(lone_##sign##bits) + offset] = {0};                            \
	unsigned char expected[] = { __VA_ARGS__ };                                                \
	struct lone_bytes bytes = { .count = sizeof(actual), .pointer = actual };                  \
	bool written = false;                                                                      \
	                                                                                           \
	written = lone_bytes_write_##sign##bits##endian(bytes, offset, constant);                  \
	                                                                                           \
	lone_test_assert_true(suite, test, written);                                               \
	lone_test_assert_true(suite, test,                                                         \
			lone_memory_is_equal(actual + offset, expected, sizeof(expected)));        \
}

#define LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_OUT_OF_BOUNDS(sign, bits, endian, alignment,            \
		constant, offset, ...)                                                             \
static LONE_TEST_FUNCTION(LONE_TYPES_BYTES_ENDIAN_TEST_FUNCTION(sign, bits,                        \
			write, endian, alignment, out_of_bounds))                                  \
{                                                                                                  \
	unsigned char actual[sizeof(lone_##sign##bits) + offset] = {0};                            \
	struct lone_bytes bytes = { .count = sizeof(actual), .pointer = actual };                  \
	bool written = false;                                                                      \
	                                                                                           \
	written = lone_bytes_write_##sign##bits##endian(bytes, sizeof(actual), constant);          \
	                                                                                           \
	lone_test_assert_false(suite, test, written);                                              \
}

LONE_TYPES_BYTES_ENDIAN_TEST_READ_WITHIN_BOUNDS(u, 16, le, aligned, 0x0102, 0, \
		0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_WITHIN_BOUNDS(u, 16, be, aligned, 0x0102, 0, \
		0x01, 0x02)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_WITHIN_BOUNDS(s, 16, le, aligned, 0x0102, 0, \
		0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_WITHIN_BOUNDS(s, 16, be, aligned, 0x0102, 0, \
		0x01, 0x02)

LONE_TYPES_BYTES_ENDIAN_TEST_READ_WITHIN_BOUNDS(u, 16, le, unaligned, 0x0102, 1, \
		0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_WITHIN_BOUNDS(u, 16, be, unaligned, 0x0203, 1, \
		0x01, 0x02, 0x03)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_WITHIN_BOUNDS(s, 16, le, unaligned, 0x0102, 1, \
		0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_WITHIN_BOUNDS(s, 16, be, unaligned, 0x0203, 1, \
		0x01, 0x02, 0x03)

LONE_TYPES_BYTES_ENDIAN_TEST_READ_WITHIN_BOUNDS(u, 32, le, aligned, 0x01020304, 0, \
		0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_WITHIN_BOUNDS(u, 32, be, aligned, 0x01020304, 0, \
		0x01, 0x02, 0x03, 0x04)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_WITHIN_BOUNDS(s, 32, le, aligned, 0x01020304, 0, \
		0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_WITHIN_BOUNDS(s, 32, be, aligned, 0x01020304, 0, \
		0x01, 0x02, 0x03, 0x04)

LONE_TYPES_BYTES_ENDIAN_TEST_READ_WITHIN_BOUNDS(u, 32, le, unaligned, 0x01020304, 1, \
		0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_WITHIN_BOUNDS(u, 32, be, unaligned, 0x02030405, 1, \
		0x01, 0x02, 0x03, 0x04, 0x05)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_WITHIN_BOUNDS(s, 32, le, unaligned, 0x01020304, 1, \
		0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_WITHIN_BOUNDS(s, 32, be, unaligned, 0x02030405, 1, \
		0x01, 0x02, 0x03, 0x04, 0x05)

LONE_TYPES_BYTES_ENDIAN_TEST_READ_WITHIN_BOUNDS(u, 64, le, aligned, 0x0102030405060708, 0, \
		0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_WITHIN_BOUNDS(u, 64, be, aligned, 0x0102030405060708, 0, \
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_WITHIN_BOUNDS(s, 64, le, aligned, 0x0102030405060708, 0, \
		0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_WITHIN_BOUNDS(s, 64, be, aligned, 0x0102030405060708, 0, \
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08)

LONE_TYPES_BYTES_ENDIAN_TEST_READ_WITHIN_BOUNDS(u, 64, le, unaligned, 0x0102030405060708, 1, \
		0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_WITHIN_BOUNDS(u, 64, be, unaligned, 0x0203040506070809, 1, \
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_WITHIN_BOUNDS(s, 64, le, unaligned, 0x0102030405060708, 1, \
		0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_WITHIN_BOUNDS(s, 64, be, unaligned, 0x0203040506070809, 1, \
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09)

LONE_TYPES_BYTES_ENDIAN_TEST_READ_OUT_OF_BOUNDS(u, 16, le, aligned, 0x0102, 0, \
		0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_OUT_OF_BOUNDS(u, 16, be, aligned, 0x0102, 0, \
		0x01, 0x02)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_OUT_OF_BOUNDS(s, 16, le, aligned, 0x0102, 0, \
		0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_OUT_OF_BOUNDS(s, 16, be, aligned, 0x0102, 0, \
		0x01, 0x02)

LONE_TYPES_BYTES_ENDIAN_TEST_READ_OUT_OF_BOUNDS(u, 16, le, unaligned, 0x0102, 1, \
		0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_OUT_OF_BOUNDS(u, 16, be, unaligned, 0x0203, 1, \
		0x01, 0x02, 0x03)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_OUT_OF_BOUNDS(s, 16, le, unaligned, 0x0102, 1, \
		0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_OUT_OF_BOUNDS(s, 16, be, unaligned, 0x0203, 1, \
		0x01, 0x02, 0x03)

LONE_TYPES_BYTES_ENDIAN_TEST_READ_OUT_OF_BOUNDS(u, 32, le, aligned, 0x01020304, 0, \
		0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_OUT_OF_BOUNDS(u, 32, be, aligned, 0x01020304, 0, \
		0x01, 0x02, 0x03, 0x04)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_OUT_OF_BOUNDS(s, 32, le, aligned, 0x01020304, 0, \
		0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_OUT_OF_BOUNDS(s, 32, be, aligned, 0x01020304, 0, \
		0x01, 0x02, 0x03, 0x04)

LONE_TYPES_BYTES_ENDIAN_TEST_READ_OUT_OF_BOUNDS(u, 32, le, unaligned, 0x01020304, 1, \
		0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_OUT_OF_BOUNDS(u, 32, be, unaligned, 0x02030405, 1, \
		0x01, 0x02, 0x03, 0x04, 0x05)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_OUT_OF_BOUNDS(s, 32, le, unaligned, 0x01020304, 1, \
		0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_OUT_OF_BOUNDS(s, 32, be, unaligned, 0x02030405, 1, \
		0x01, 0x02, 0x03, 0x04, 0x05)

LONE_TYPES_BYTES_ENDIAN_TEST_READ_OUT_OF_BOUNDS(u, 64, le, aligned, 0x0102030405060708, 0, \
		0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_OUT_OF_BOUNDS(u, 64, be, aligned, 0x0102030405060708, 0, \
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_OUT_OF_BOUNDS(s, 64, le, aligned, 0x0102030405060708, 0, \
		0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_OUT_OF_BOUNDS(s, 64, be, aligned, 0x0102030405060708, 0, \
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08)

LONE_TYPES_BYTES_ENDIAN_TEST_READ_OUT_OF_BOUNDS(u, 64, le, unaligned, 0x0102030405060708, 1, \
		0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_OUT_OF_BOUNDS(u, 64, be, unaligned, 0x0203040506070809, 1, \
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_OUT_OF_BOUNDS(s, 64, le, unaligned, 0x0102030405060708, 1, \
		0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_READ_OUT_OF_BOUNDS(s, 64, be, unaligned, 0x0203040506070809, 1, \
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09)

LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_WITHIN_BOUNDS(u, 16, le, aligned, 0x0102, 0, \
		0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_WITHIN_BOUNDS(u, 16, be, aligned, 0x0102, 0, \
		0x01, 0x02)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_WITHIN_BOUNDS(s, 16, le, aligned, 0x0102, 0, \
		0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_WITHIN_BOUNDS(s, 16, be, aligned, 0x0102, 0, \
		0x01, 0x02)

LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_WITHIN_BOUNDS(u, 16, le, unaligned, 0x0102, 1, \
		0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_WITHIN_BOUNDS(u, 16, be, unaligned, 0x0102, 1, \
		0x01, 0x02)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_WITHIN_BOUNDS(s, 16, le, unaligned, 0x0102, 1, \
		0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_WITHIN_BOUNDS(s, 16, be, unaligned, 0x0102, 1, \
		0x01, 0x02)

LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_WITHIN_BOUNDS(u, 32, le, aligned, 0x01020304, 0, \
		0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_WITHIN_BOUNDS(u, 32, be, aligned, 0x01020304, 0, \
		0x01, 0x02, 0x03, 0x04)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_WITHIN_BOUNDS(s, 32, le, aligned, 0x01020304, 0, \
		0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_WITHIN_BOUNDS(s, 32, be, aligned, 0x01020304, 0, \
		0x01, 0x02, 0x03, 0x04)

LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_WITHIN_BOUNDS(u, 32, le, unaligned, 0x01020304, 1, \
		0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_WITHIN_BOUNDS(u, 32, be, unaligned, 0x01020304, 1, \
		0x01, 0x02, 0x03, 0x04)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_WITHIN_BOUNDS(s, 32, le, unaligned, 0x01020304, 1, \
		0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_WITHIN_BOUNDS(s, 32, be, unaligned, 0x01020304, 1, \
		0x01, 0x02, 0x03, 0x04)

LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_WITHIN_BOUNDS(u, 64, le, aligned, 0x0102030405060708, 0, \
		0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_WITHIN_BOUNDS(u, 64, be, aligned, 0x0102030405060708, 0, \
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_WITHIN_BOUNDS(s, 64, le, aligned, 0x0102030405060708, 0, \
		0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_WITHIN_BOUNDS(s, 64, be, aligned, 0x0102030405060708, 0, \
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08)

LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_WITHIN_BOUNDS(u, 64, le, unaligned, 0x0102030405060708, 1, \
		0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_WITHIN_BOUNDS(u, 64, be, unaligned, 0x0102030405060708, 1, \
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_WITHIN_BOUNDS(s, 64, le, unaligned, 0x0102030405060708, 1, \
		0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_WITHIN_BOUNDS(s, 64, be, unaligned, 0x0102030405060708, 1, \
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08)

LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_OUT_OF_BOUNDS(u, 16, le, aligned, 0x0102, 0, \
		0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_OUT_OF_BOUNDS(u, 16, be, aligned, 0x0102, 0, \
		0x01, 0x02)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_OUT_OF_BOUNDS(s, 16, le, aligned, 0x0102, 0, \
		0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_OUT_OF_BOUNDS(s, 16, be, aligned, 0x0102, 0, \
		0x01, 0x02)

LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_OUT_OF_BOUNDS(u, 16, le, unaligned, 0x0102, 1, \
		0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_OUT_OF_BOUNDS(u, 16, be, unaligned, 0x0102, 1, \
		0x01, 0x02)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_OUT_OF_BOUNDS(s, 16, le, unaligned, 0x0102, 1, \
		0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_OUT_OF_BOUNDS(s, 16, be, unaligned, 0x0102, 1, \
		0x01, 0x02)

LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_OUT_OF_BOUNDS(u, 32, le, aligned, 0x01020304, 0, \
		0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_OUT_OF_BOUNDS(u, 32, be, aligned, 0x01020304, 0, \
		0x01, 0x02, 0x03, 0x04)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_OUT_OF_BOUNDS(s, 32, le, aligned, 0x01020304, 0, \
		0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_OUT_OF_BOUNDS(s, 32, be, aligned, 0x01020304, 0, \
		0x01, 0x02, 0x03, 0x04)

LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_OUT_OF_BOUNDS(u, 32, le, unaligned, 0x01020304, 1, \
		0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_OUT_OF_BOUNDS(u, 32, be, unaligned, 0x01020304, 1, \
		0x01, 0x02, 0x03, 0x04)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_OUT_OF_BOUNDS(s, 32, le, unaligned, 0x01020304, 1, \
		0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_OUT_OF_BOUNDS(s, 32, be, unaligned, 0x01020304, 1, \
		0x01, 0x02, 0x03, 0x04)

LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_OUT_OF_BOUNDS(u, 64, le, aligned, 0x0102030405060708, 0, \
		0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_OUT_OF_BOUNDS(u, 64, be, aligned, 0x0102030405060708, 0, \
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_OUT_OF_BOUNDS(s, 64, le, aligned, 0x0102030405060708, 0, \
		0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_OUT_OF_BOUNDS(s, 64, be, aligned, 0x0102030405060708, 0, \
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08)

LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_OUT_OF_BOUNDS(u, 64, le, unaligned, 0x0102030405060708, 1, \
		0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_OUT_OF_BOUNDS(u, 64, be, unaligned, 0x0102030405060708, 1, \
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_OUT_OF_BOUNDS(s, 64, le, unaligned, 0x0102030405060708, 1, \
		0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01)
LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_OUT_OF_BOUNDS(s, 64, be, unaligned, 0x0102030405060708, 1, \
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08)

#undef LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_OUT_OF_BOUNDS
#undef LONE_TYPES_BYTES_ENDIAN_TEST_WRITE_WITHIN_BOUNDS
#undef LONE_TYPES_BYTES_ENDIAN_TEST_READ_OUT_OF_BOUNDS
#undef LONE_TYPES_BYTES_ENDIAN_TEST_READ_WITHIN_BOUNDS

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

#define LONE_TYPES_BYTES_ENDIAN_TEST_CASE_6(sign, bits, operation, endian, alignment, bounds)      \
	LONE_TEST_CASE(                                                                            \
		LONE_TYPES_BYTES_ENDIAN_TEST_NAME(sign, bits, operation,                           \
			endian, alignment, bounds),                                                \
		LONE_TYPES_BYTES_ENDIAN_TEST_FUNCTION(sign, bits, operation,                       \
			endian, alignment, bounds)),

#define LONE_TYPES_BYTES_ENDIAN_TEST_CASE_5(sign, bits, operation, endian, alignment)              \
	LONE_TYPES_BYTES_ENDIAN_TEST_CASE_6(sign, bits, operation,                                 \
		endian, alignment, within_bounds)                                                  \
	LONE_TYPES_BYTES_ENDIAN_TEST_CASE_6(sign, bits, operation,                                 \
		endian, alignment, out_of_bounds)

#define LONE_TYPES_BYTES_ENDIAN_TEST_CASE_4(sign, bits, operation, endian)                         \
	LONE_TYPES_BYTES_ENDIAN_TEST_CASE_5(sign, bits, operation, endian, aligned)                \
	LONE_TYPES_BYTES_ENDIAN_TEST_CASE_5(sign, bits, operation, endian, unaligned)

#define LONE_TYPES_BYTES_ENDIAN_TEST_CASE_3(sign, bits, operation)                                 \
	LONE_TYPES_BYTES_ENDIAN_TEST_CASE_4(sign, bits, operation, le)                             \
	LONE_TYPES_BYTES_ENDIAN_TEST_CASE_4(sign, bits, operation, be)

#define LONE_TYPES_BYTES_ENDIAN_TEST_CASE_2(sign, bits)                                            \
	LONE_TYPES_BYTES_ENDIAN_TEST_CASE_3(sign, bits, read)                                      \
	LONE_TYPES_BYTES_ENDIAN_TEST_CASE_3(sign, bits, write)

#define LONE_TYPES_BYTES_ENDIAN_TEST_CASE_1(sign)                                                  \
	LONE_TYPES_BYTES_ENDIAN_TEST_CASE_2(sign, 16)                                              \
	LONE_TYPES_BYTES_ENDIAN_TEST_CASE_2(sign, 32)                                              \
	LONE_TYPES_BYTES_ENDIAN_TEST_CASE_2(sign, 64)

#define LONE_TYPES_BYTES_ENDIAN_TEST_CASES()                                                       \
	LONE_TYPES_BYTES_ENDIAN_TEST_CASE_1(u)                                                     \
	LONE_TYPES_BYTES_ENDIAN_TEST_CASE_1(s)

		LONE_TYPES_BYTES_ENDIAN_TEST_CASES()

#undef LONE_TYPES_BYTES_ENDIAN_TEST_CASES
#undef LONE_TYPES_BYTES_ENDIAN_TEST_CASE_1
#undef LONE_TYPES_BYTES_ENDIAN_TEST_CASE_2
#undef LONE_TYPES_BYTES_ENDIAN_TEST_CASE_3
#undef LONE_TYPES_BYTES_ENDIAN_TEST_CASE_4
#undef LONE_TYPES_BYTES_ENDIAN_TEST_CASE_5
#undef LONE_TYPES_BYTES_ENDIAN_TEST_CASE_6

		LONE_TEST_CASE_NULL(),
	};

	struct lone_test_suite suite = LONE_TEST_SUITE(cases);
	enum lone_test_result result;

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

#undef LONE_TYPES_BYTES_ENDIAN_TEST_NAME
#undef LONE_TYPES_BYTES_ENDIAN_TEST_FUNCTION

#undef LONE_TYPES_ENDIAN_TEST_NAME
#undef LONE_TYPES_ENDIAN_TEST_FUNCTION

#undef LONE_TYPES_BYTES_TEST_NAME
#undef LONE_TYPES_BYTES_TEST_FUNCTION

#undef LONE_TYPES_TEST_NAME
#undef LONE_TYPES_TEST_FUNCTION

#include <lone/architecture/linux/entry_point.c>
