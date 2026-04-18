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

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Edge value tests for read/write at type boundaries.                 │
   │                                                                        │
   │    Tests zero, one, max for unsigned types.                            │
   │    Tests zero, one, minus_one, min, max for signed types.              │
   │    Tests in all four configurations: read/write × aligned/unaligned.   │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

#define LONE_TYPES_EDGE_TEST_NAME(sign, bits, operation, alignment, variant) \
	"lone/types/" #sign #bits "/" #operation "/" #alignment "/" #variant

#define LONE_TYPES_EDGE_TEST_FUNCTION(sign, bits, operation, alignment, variant) \
	test_lone_types_lone_##sign##bits##_##operation##_##alignment##_##variant

#define LONE_TYPES_EDGE_TEST_READ_ALIGNED(sign, bits, variant, constant)                              \
static LONE_TEST_FUNCTION(LONE_TYPES_EDGE_TEST_FUNCTION(sign, bits, read, aligned, variant))          \
{                                                                                                     \
	lone_##sign##bits value = constant;                                                           \
	                                                                                              \
	lone_test_assert_##sign##bits##_equal(suite, test,                                            \
			lone_##sign##bits##_read(&value), constant);                                  \
}

#define LONE_TYPES_EDGE_TEST_READ_UNALIGNED(sign, bits, variant, constant)                            \
static LONE_TEST_FUNCTION(LONE_TYPES_EDGE_TEST_FUNCTION(sign, bits, read, unaligned, variant))        \
{                                                                                                     \
	struct PACKED(bits) {                                                                         \
		unsigned char byte;                                                                   \
		lone_##sign##bits value;                                                              \
	} unaligned = { 0, constant };                                                                \
	                                                                                              \
	lone_test_assert_##sign##bits##_equal(suite, test,                                            \
			lone_##sign##bits##_read(&unaligned.value), constant);                        \
}

#define LONE_TYPES_EDGE_TEST_WRITE_ALIGNED(sign, bits, variant, constant)                             \
static LONE_TEST_FUNCTION(LONE_TYPES_EDGE_TEST_FUNCTION(sign, bits, write, aligned, variant))         \
{                                                                                                     \
	lone_##sign##bits value = 0;                                                                  \
	                                                                                              \
	lone_##sign##bits##_write(&value, constant);                                                  \
	                                                                                              \
	lone_test_assert_##sign##bits##_equal(suite, test, value, constant);                          \
}

#define LONE_TYPES_EDGE_TEST_WRITE_UNALIGNED(sign, bits, variant, constant)                           \
static LONE_TEST_FUNCTION(LONE_TYPES_EDGE_TEST_FUNCTION(sign, bits, write, unaligned, variant))       \
{                                                                                                     \
	struct PACKED(bits) {                                                                         \
		unsigned char byte;                                                                   \
		lone_##sign##bits value;                                                              \
	} unaligned = {0};                                                                            \
	                                                                                              \
	lone_##sign##bits##_write(&unaligned.value, constant);                                        \
	                                                                                              \
	lone_test_assert_##sign##bits##_equal(suite, test, unaligned.value, constant);                \
}

#define LONE_TYPES_EDGE_TESTS(sign, bits, variant, constant)                                          \
	LONE_TYPES_EDGE_TEST_READ_ALIGNED(sign, bits, variant, constant)                              \
	LONE_TYPES_EDGE_TEST_READ_UNALIGNED(sign, bits, variant, constant)                            \
	LONE_TYPES_EDGE_TEST_WRITE_ALIGNED(sign, bits, variant, constant)                             \
	LONE_TYPES_EDGE_TEST_WRITE_UNALIGNED(sign, bits, variant, constant)

LONE_TYPES_EDGE_TESTS(u, 8,  zero,         0)
LONE_TYPES_EDGE_TESTS(u, 8,  one,         +1)
LONE_TYPES_EDGE_TESTS(u, 8,  max,       +255)
LONE_TYPES_EDGE_TESTS(s, 8,  zero,         0)
LONE_TYPES_EDGE_TESTS(s, 8,  one,         +1)
LONE_TYPES_EDGE_TESTS(s, 8,  minus_one,   -1)
LONE_TYPES_EDGE_TESTS(s, 8,  max,       +127)
LONE_TYPES_EDGE_TESTS(s, 8,  min,       -128)

LONE_TYPES_EDGE_TESTS(u, 16, zero,           0)
LONE_TYPES_EDGE_TESTS(u, 16, one,           +1)
LONE_TYPES_EDGE_TESTS(u, 16, max,       +65535)
LONE_TYPES_EDGE_TESTS(s, 16, zero,           0)
LONE_TYPES_EDGE_TESTS(s, 16, one,           +1)
LONE_TYPES_EDGE_TESTS(s, 16, minus_one,     -1)
LONE_TYPES_EDGE_TESTS(s, 16, max,       +32767)
LONE_TYPES_EDGE_TESTS(s, 16, min,       -32768)

LONE_TYPES_EDGE_TESTS(u, 32, zero,                      0)
LONE_TYPES_EDGE_TESTS(u, 32, one,                      +1)
LONE_TYPES_EDGE_TESTS(u, 32, max,           +4294967295UL)
LONE_TYPES_EDGE_TESTS(s, 32, zero,                      0)
LONE_TYPES_EDGE_TESTS(s, 32, one,                      +1)
LONE_TYPES_EDGE_TESTS(s, 32, minus_one,                -1)
LONE_TYPES_EDGE_TESTS(s, 32, max,             +2147483647)
LONE_TYPES_EDGE_TESTS(s, 32, min,       (-2147483647 - 1))

LONE_TYPES_EDGE_TESTS(u, 64, zero,                                0)
LONE_TYPES_EDGE_TESTS(u, 64, one,                                +1)
LONE_TYPES_EDGE_TESTS(u, 64, max,            18446744073709551615UL)
LONE_TYPES_EDGE_TESTS(s, 64, zero,                                0)
LONE_TYPES_EDGE_TESTS(s, 64, one,                                +1)
LONE_TYPES_EDGE_TESTS(s, 64, minus_one,                          -1)
LONE_TYPES_EDGE_TESTS(s, 64, max,             +9223372036854775807L)
LONE_TYPES_EDGE_TESTS(s, 64, min,       (-9223372036854775807L - 1))

#undef LONE_TYPES_EDGE_TESTS
#undef LONE_TYPES_EDGE_TEST_READ_ALIGNED
#undef LONE_TYPES_EDGE_TEST_READ_UNALIGNED
#undef LONE_TYPES_EDGE_TEST_WRITE_ALIGNED
#undef LONE_TYPES_EDGE_TEST_WRITE_UNALIGNED

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Round-trip tests: write then read back from the same address.       │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

#define LONE_TYPES_ROUND_TRIP_TEST_NAME(sign, bits, alignment) \
	"lone/types/" #sign #bits "/round-trip/" #alignment

#define LONE_TYPES_ROUND_TRIP_TEST_FUNCTION(sign, bits, alignment) \
	test_lone_types_lone_##sign##bits##_roundtrip_##alignment

#define LONE_TYPES_ROUND_TRIP_TEST_ALIGNED(sign, bits, constant)                                      \
static LONE_TEST_FUNCTION(LONE_TYPES_ROUND_TRIP_TEST_FUNCTION(sign, bits, aligned))                   \
{                                                                                                     \
	lone_##sign##bits storage = 0;                                                                \
	                                                                                              \
	lone_##sign##bits##_write(&storage, constant);                                                \
	                                                                                              \
	lone_test_assert_##sign##bits##_equal(suite, test,                                            \
			lone_##sign##bits##_read(&storage), constant);                                \
}

#define LONE_TYPES_ROUND_TRIP_TEST_UNALIGNED(sign, bits, constant)                                    \
static LONE_TEST_FUNCTION(LONE_TYPES_ROUND_TRIP_TEST_FUNCTION(sign, bits, unaligned))                 \
{                                                                                                     \
	struct PACKED(bits) {                                                                         \
		unsigned char byte;                                                                   \
		lone_##sign##bits value;                                                              \
	} unaligned = {0};                                                                            \
	                                                                                              \
	lone_##sign##bits##_write(&unaligned.value, constant);                                        \
	                                                                                              \
	lone_test_assert_##sign##bits##_equal(suite, test,                                            \
			lone_##sign##bits##_read(&unaligned.value), constant);                        \
}

LONE_TYPES_ROUND_TRIP_TEST_ALIGNED(u,  8,                    +214)
LONE_TYPES_ROUND_TRIP_TEST_ALIGNED(s,  8,                     -42)
LONE_TYPES_ROUND_TRIP_TEST_ALIGNED(u, 16,                  +56535)
LONE_TYPES_ROUND_TRIP_TEST_ALIGNED(s, 16,                   -9001)
LONE_TYPES_ROUND_TRIP_TEST_ALIGNED(u, 32,           +4294927296UL)
LONE_TYPES_ROUND_TRIP_TEST_ALIGNED(s, 32,                  -40000)
LONE_TYPES_ROUND_TRIP_TEST_ALIGNED(u, 64, +17212176182698430302UL)
LONE_TYPES_ROUND_TRIP_TEST_ALIGNED(s, 64,    -1234567891011121314)

LONE_TYPES_ROUND_TRIP_TEST_UNALIGNED(u,  8,                    +214)
LONE_TYPES_ROUND_TRIP_TEST_UNALIGNED(s,  8,                     -42)
LONE_TYPES_ROUND_TRIP_TEST_UNALIGNED(u, 16,                  +56535)
LONE_TYPES_ROUND_TRIP_TEST_UNALIGNED(s, 16,                   -9001)
LONE_TYPES_ROUND_TRIP_TEST_UNALIGNED(u, 32,           +4294927296UL)
LONE_TYPES_ROUND_TRIP_TEST_UNALIGNED(s, 32,                  -40000)
LONE_TYPES_ROUND_TRIP_TEST_UNALIGNED(u, 64, +17212176182698430302UL)
LONE_TYPES_ROUND_TRIP_TEST_UNALIGNED(s, 64,    -1234567891011121314)

#undef LONE_TYPES_ROUND_TRIP_TEST_ALIGNED
#undef LONE_TYPES_ROUND_TRIP_TEST_UNALIGNED

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    No-clobber tests: verify unaligned writes do not corrupt            │
   │    adjacent memory.                                                    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

#define LONE_TYPES_NO_CLOBBER_TEST_NAME(sign, bits) \
	"lone/types/" #sign #bits "/write/unaligned/no-clobber"

#define LONE_TYPES_NO_CLOBBER_TEST_FUNCTION(sign, bits) \
	test_lone_types_lone_##sign##bits##_write_unaligned_no_clobber

#define LONE_TYPES_NO_CLOBBER_TEST(sign, bits, constant)                                               \
static LONE_TEST_FUNCTION(LONE_TYPES_NO_CLOBBER_TEST_FUNCTION(sign, bits))                             \
{                                                                                                     \
	struct __attribute__((packed)) {                                                               \
		unsigned char before;                                                                 \
		lone_##sign##bits value;                                                              \
		unsigned char after;                                                                  \
	} buf = { 0xAA, 0, 0xBB };                                                                   \
	                                                                                              \
	lone_##sign##bits##_write(&buf.value, constant);                                              \
	                                                                                              \
	lone_test_assert_u8_equal(suite, test, buf.before, 0xAA);                                     \
	lone_test_assert_u8_equal(suite, test, buf.after, 0xBB);                                      \
	lone_test_assert_##sign##bits##_equal(suite, test, buf.value, constant);                      \
}

LONE_TYPES_NO_CLOBBER_TEST(u, 16, 56535)
LONE_TYPES_NO_CLOBBER_TEST(s, 16, -9001)
LONE_TYPES_NO_CLOBBER_TEST(u, 32, 4294927296UL)
LONE_TYPES_NO_CLOBBER_TEST(s, 32, -40000)
LONE_TYPES_NO_CLOBBER_TEST(u, 64, 17212176182698430302UL)
LONE_TYPES_NO_CLOBBER_TEST(s, 64, -1234567891011121314)

#undef LONE_TYPES_NO_CLOBBER_TEST

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Multi-offset tests: verify reads and writes at alignment            │
   │    offsets beyond +1 for multi-byte types.                             │
   │                                                                        │
   │    Existing unaligned tests use offset 1 for one leading byte.         │
   │    These test offsets 3, 5, and 7 which exercise different             │
   │    partial-word boundary crossings.                                    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

#define LONE_TYPES_OFFSET_TEST_NAME(sign, bits, operation, offset) \
	"lone/types/" #sign #bits "/" #operation "/offset/" #offset

#define LONE_TYPES_OFFSET_TEST_FUNCTION(sign, bits, operation, offset) \
	test_lone_types_lone_##sign##bits##_##operation##_offset_##offset

#define LONE_TYPES_OFFSET_TEST_READ(sign, bits, offset, constant)                                     \
static LONE_TEST_FUNCTION(LONE_TYPES_OFFSET_TEST_FUNCTION(sign, bits, read, offset))                  \
{                                                                                                     \
	struct __attribute__((packed)) {                                                               \
		unsigned char padding[offset];                                                        \
		lone_##sign##bits value;                                                              \
	} unaligned = {{0}, constant};                                                                \
	                                                                                              \
	lone_test_assert_##sign##bits##_equal(suite, test,                                            \
			lone_##sign##bits##_read(&unaligned.value), constant);                        \
}

#define LONE_TYPES_OFFSET_TEST_WRITE(sign, bits, offset, constant)                                    \
static LONE_TEST_FUNCTION(LONE_TYPES_OFFSET_TEST_FUNCTION(sign, bits, write, offset))                 \
{                                                                                                     \
	struct __attribute__((packed)) {                                                               \
		unsigned char padding[offset];                                                        \
		lone_##sign##bits value;                                                              \
	} unaligned = {{0}, 0};                                                                       \
	                                                                                              \
	lone_##sign##bits##_write(&unaligned.value, constant);                                        \
	                                                                                              \
	lone_test_assert_##sign##bits##_equal(suite, test, unaligned.value, constant);                \
}

#define LONE_TYPES_OFFSET_TESTS(sign, bits, offset, constant)                                         \
	LONE_TYPES_OFFSET_TEST_READ(sign, bits, offset, constant)                                     \
	LONE_TYPES_OFFSET_TEST_WRITE(sign, bits, offset, constant)

LONE_TYPES_OFFSET_TESTS(u, 16, 3, +56535)
LONE_TYPES_OFFSET_TESTS(s, 16, 3,  -9001)

LONE_TYPES_OFFSET_TESTS(u, 32, 3, +4294927296UL)
LONE_TYPES_OFFSET_TESTS(s, 32, 3,        -40000)

LONE_TYPES_OFFSET_TESTS(u, 64, 3, +17212176182698430302UL)
LONE_TYPES_OFFSET_TESTS(s, 64, 3,    -1234567891011121314)
LONE_TYPES_OFFSET_TESTS(u, 64, 5, +17212176182698430302UL)
LONE_TYPES_OFFSET_TESTS(s, 64, 5,    -1234567891011121314)
LONE_TYPES_OFFSET_TESTS(u, 64, 7, +17212176182698430302UL)
LONE_TYPES_OFFSET_TESTS(s, 64, 7,    -1234567891011121314)

#undef LONE_TYPES_OFFSET_TESTS
#undef LONE_TYPES_OFFSET_TEST_READ
#undef LONE_TYPES_OFFSET_TEST_WRITE

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

LONE_TYPES_BYTES_TEST_READ_ALIGNED_WITHIN_BOUNDS(u,  8, +214)
LONE_TYPES_BYTES_TEST_READ_ALIGNED_WITHIN_BOUNDS(s,  8, -42)
LONE_TYPES_BYTES_TEST_READ_ALIGNED_WITHIN_BOUNDS(u, 16, +56535)
LONE_TYPES_BYTES_TEST_READ_ALIGNED_WITHIN_BOUNDS(s, 16, -9001)
LONE_TYPES_BYTES_TEST_READ_ALIGNED_WITHIN_BOUNDS(u, 32, +4294927296)
LONE_TYPES_BYTES_TEST_READ_ALIGNED_WITHIN_BOUNDS(s, 32, -40000)
LONE_TYPES_BYTES_TEST_READ_ALIGNED_WITHIN_BOUNDS(u, 64, +17212176182698430302UL)
LONE_TYPES_BYTES_TEST_READ_ALIGNED_WITHIN_BOUNDS(s, 64, -1234567891011121314)

LONE_TYPES_BYTES_TEST_READ_ALIGNED_OUT_OF_BOUNDS(u,  8, +214)
LONE_TYPES_BYTES_TEST_READ_ALIGNED_OUT_OF_BOUNDS(s,  8, -42)
LONE_TYPES_BYTES_TEST_READ_ALIGNED_OUT_OF_BOUNDS(u, 16, +56535)
LONE_TYPES_BYTES_TEST_READ_ALIGNED_OUT_OF_BOUNDS(s, 16, -9001)
LONE_TYPES_BYTES_TEST_READ_ALIGNED_OUT_OF_BOUNDS(u, 32, +4294927296)
LONE_TYPES_BYTES_TEST_READ_ALIGNED_OUT_OF_BOUNDS(s, 32, -40000)
LONE_TYPES_BYTES_TEST_READ_ALIGNED_OUT_OF_BOUNDS(u, 64, +17212176182698430302UL)
LONE_TYPES_BYTES_TEST_READ_ALIGNED_OUT_OF_BOUNDS(s, 64, -1234567891011121314)

LONE_TYPES_BYTES_TEST_READ_UNALIGNED_WITHIN_BOUNDS(u,  8, +214)
LONE_TYPES_BYTES_TEST_READ_UNALIGNED_WITHIN_BOUNDS(s,  8, -42)
LONE_TYPES_BYTES_TEST_READ_UNALIGNED_WITHIN_BOUNDS(u, 16, +56535)
LONE_TYPES_BYTES_TEST_READ_UNALIGNED_WITHIN_BOUNDS(s, 16, -9001)
LONE_TYPES_BYTES_TEST_READ_UNALIGNED_WITHIN_BOUNDS(u, 32, +4294927296)
LONE_TYPES_BYTES_TEST_READ_UNALIGNED_WITHIN_BOUNDS(s, 32, -40000)
LONE_TYPES_BYTES_TEST_READ_UNALIGNED_WITHIN_BOUNDS(u, 64, +17212176182698430302UL)
LONE_TYPES_BYTES_TEST_READ_UNALIGNED_WITHIN_BOUNDS(s, 64, -1234567891011121314)

LONE_TYPES_BYTES_TEST_READ_UNALIGNED_OUT_OF_BOUNDS(u,  8, +214)
LONE_TYPES_BYTES_TEST_READ_UNALIGNED_OUT_OF_BOUNDS(s,  8, -42)
LONE_TYPES_BYTES_TEST_READ_UNALIGNED_OUT_OF_BOUNDS(u, 16, +56535)
LONE_TYPES_BYTES_TEST_READ_UNALIGNED_OUT_OF_BOUNDS(s, 16, -9001)
LONE_TYPES_BYTES_TEST_READ_UNALIGNED_OUT_OF_BOUNDS(u, 32, +4294927296)
LONE_TYPES_BYTES_TEST_READ_UNALIGNED_OUT_OF_BOUNDS(s, 32, -40000)
LONE_TYPES_BYTES_TEST_READ_UNALIGNED_OUT_OF_BOUNDS(u, 64, +17212176182698430302UL)
LONE_TYPES_BYTES_TEST_READ_UNALIGNED_OUT_OF_BOUNDS(s, 64, -1234567891011121314)

LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_WITHIN_BOUNDS(u,  8, +214)
LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_WITHIN_BOUNDS(s,  8, -42)
LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_WITHIN_BOUNDS(u, 16, +56535)
LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_WITHIN_BOUNDS(s, 16, -9001)
LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_WITHIN_BOUNDS(u, 32, +4294927296)
LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_WITHIN_BOUNDS(s, 32, -40000)
LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_WITHIN_BOUNDS(u, 64, +17212176182698430302UL)
LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_WITHIN_BOUNDS(s, 64, -1234567891011121314)

LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_OUT_OF_BOUNDS(u,  8, +214)
LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_OUT_OF_BOUNDS(s,  8, -42)
LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_OUT_OF_BOUNDS(u, 16, +56535)
LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_OUT_OF_BOUNDS(s, 16, -9001)
LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_OUT_OF_BOUNDS(u, 32, +4294927296)
LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_OUT_OF_BOUNDS(s, 32, -40000)
LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_OUT_OF_BOUNDS(u, 64, +17212176182698430302UL)
LONE_TYPES_BYTES_TEST_WRITE_ALIGNED_OUT_OF_BOUNDS(s, 64, -1234567891011121314)

LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_WITHIN_BOUNDS(u,  8, +214)
LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_WITHIN_BOUNDS(s,  8, -42)
LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_WITHIN_BOUNDS(u, 16, +56535)
LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_WITHIN_BOUNDS(s, 16, -9001)
LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_WITHIN_BOUNDS(u, 32, +4294927296)
LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_WITHIN_BOUNDS(s, 32, -40000)
LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_WITHIN_BOUNDS(u, 64, +17212176182698430302UL)
LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_WITHIN_BOUNDS(s, 64, -1234567891011121314)

LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_OUT_OF_BOUNDS(u,  8, +214)
LONE_TYPES_BYTES_TEST_WRITE_UNALIGNED_OUT_OF_BOUNDS(s,  8, -42)
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
	LONE_TYPES_TEST_CASE_2(sign,  8)                                                           \
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

#define LONE_TYPES_EDGE_TEST_CASE(sign, bits, operation, alignment, variant)                       \
	LONE_TEST_CASE(LONE_TYPES_EDGE_TEST_NAME(sign, bits, operation, alignment, variant),       \
		LONE_TYPES_EDGE_TEST_FUNCTION(sign, bits, operation, alignment, variant)),

#define LONE_TYPES_EDGE_TEST_ALL_OPERATIONS(sign, bits, variant)                                   \
	LONE_TYPES_EDGE_TEST_CASE(sign, bits, read,  aligned,   variant)                           \
	LONE_TYPES_EDGE_TEST_CASE(sign, bits, read,  unaligned, variant)                           \
	LONE_TYPES_EDGE_TEST_CASE(sign, bits, write, aligned,   variant)                           \
	LONE_TYPES_EDGE_TEST_CASE(sign, bits, write, unaligned, variant)

#define LONE_TYPES_EDGE_TEST_UNSIGNED(sign, bits)                                                  \
	LONE_TYPES_EDGE_TEST_ALL_OPERATIONS(sign, bits, zero)                                      \
	LONE_TYPES_EDGE_TEST_ALL_OPERATIONS(sign, bits, one)                                       \
	LONE_TYPES_EDGE_TEST_ALL_OPERATIONS(sign, bits, max)

#define LONE_TYPES_EDGE_TEST_SIGNED(sign, bits)                                                    \
	LONE_TYPES_EDGE_TEST_ALL_OPERATIONS(sign, bits, zero)                                      \
	LONE_TYPES_EDGE_TEST_ALL_OPERATIONS(sign, bits, one)                                       \
	LONE_TYPES_EDGE_TEST_ALL_OPERATIONS(sign, bits, minus_one)                                 \
	LONE_TYPES_EDGE_TEST_ALL_OPERATIONS(sign, bits, max)                                       \
	LONE_TYPES_EDGE_TEST_ALL_OPERATIONS(sign, bits, min)

#define LONE_TYPES_EDGE_TEST_CASES()                                                               \
	LONE_TYPES_EDGE_TEST_UNSIGNED(u,  8)                                                       \
	LONE_TYPES_EDGE_TEST_SIGNED(s,    8)                                                       \
	LONE_TYPES_EDGE_TEST_UNSIGNED(u, 16)                                                       \
	LONE_TYPES_EDGE_TEST_SIGNED(s,   16)                                                       \
	LONE_TYPES_EDGE_TEST_UNSIGNED(u, 32)                                                       \
	LONE_TYPES_EDGE_TEST_SIGNED(s,   32)                                                       \
	LONE_TYPES_EDGE_TEST_UNSIGNED(u, 64)                                                       \
	LONE_TYPES_EDGE_TEST_SIGNED(s,   64)

		LONE_TYPES_EDGE_TEST_CASES()

#undef LONE_TYPES_EDGE_TEST_CASES
#undef LONE_TYPES_EDGE_TEST_UNSIGNED
#undef LONE_TYPES_EDGE_TEST_SIGNED
#undef LONE_TYPES_EDGE_TEST_ALL_OPERATIONS
#undef LONE_TYPES_EDGE_TEST_CASE

#define LONE_TYPES_ROUND_TRIP_TEST_CASE(sign, bits, alignment)                                     \
	LONE_TEST_CASE(LONE_TYPES_ROUND_TRIP_TEST_NAME(sign, bits, alignment),                     \
		LONE_TYPES_ROUND_TRIP_TEST_FUNCTION(sign, bits, alignment)),

#define LONE_TYPES_ROUND_TRIP_TEST_CASES_2(sign, bits)                                             \
	LONE_TYPES_ROUND_TRIP_TEST_CASE(sign, bits, aligned)                                       \
	LONE_TYPES_ROUND_TRIP_TEST_CASE(sign, bits, unaligned)

#define LONE_TYPES_ROUND_TRIP_TEST_CASES()                                                         \
	LONE_TYPES_ROUND_TRIP_TEST_CASES_2(u,  8)                                                  \
	LONE_TYPES_ROUND_TRIP_TEST_CASES_2(s,  8)                                                  \
	LONE_TYPES_ROUND_TRIP_TEST_CASES_2(u, 16)                                                  \
	LONE_TYPES_ROUND_TRIP_TEST_CASES_2(s, 16)                                                  \
	LONE_TYPES_ROUND_TRIP_TEST_CASES_2(u, 32)                                                  \
	LONE_TYPES_ROUND_TRIP_TEST_CASES_2(s, 32)                                                  \
	LONE_TYPES_ROUND_TRIP_TEST_CASES_2(u, 64)                                                  \
	LONE_TYPES_ROUND_TRIP_TEST_CASES_2(s, 64)

		LONE_TYPES_ROUND_TRIP_TEST_CASES()

#undef LONE_TYPES_ROUND_TRIP_TEST_CASES
#undef LONE_TYPES_ROUND_TRIP_TEST_CASES_2
#undef LONE_TYPES_ROUND_TRIP_TEST_CASE

#define LONE_TYPES_NO_CLOBBER_TEST_CASE(sign, bits)                                                \
	LONE_TEST_CASE(LONE_TYPES_NO_CLOBBER_TEST_NAME(sign, bits),                                \
		LONE_TYPES_NO_CLOBBER_TEST_FUNCTION(sign, bits)),

#define LONE_TYPES_NO_CLOBBER_TEST_CASES()                                                         \
	LONE_TYPES_NO_CLOBBER_TEST_CASE(u, 16)                                                     \
	LONE_TYPES_NO_CLOBBER_TEST_CASE(s, 16)                                                     \
	LONE_TYPES_NO_CLOBBER_TEST_CASE(u, 32)                                                     \
	LONE_TYPES_NO_CLOBBER_TEST_CASE(s, 32)                                                     \
	LONE_TYPES_NO_CLOBBER_TEST_CASE(u, 64)                                                     \
	LONE_TYPES_NO_CLOBBER_TEST_CASE(s, 64)

		LONE_TYPES_NO_CLOBBER_TEST_CASES()

#undef LONE_TYPES_NO_CLOBBER_TEST_CASES
#undef LONE_TYPES_NO_CLOBBER_TEST_CASE

#define LONE_TYPES_OFFSET_TEST_CASE(sign, bits, operation, offset)                                 \
	LONE_TEST_CASE(LONE_TYPES_OFFSET_TEST_NAME(sign, bits, operation, offset),                 \
		LONE_TYPES_OFFSET_TEST_FUNCTION(sign, bits, operation, offset)),

#define LONE_TYPES_OFFSET_TEST_ALL(sign, bits, offset)                                             \
	LONE_TYPES_OFFSET_TEST_CASE(sign, bits, read, offset)                                      \
	LONE_TYPES_OFFSET_TEST_CASE(sign, bits, write, offset)

#define LONE_TYPES_OFFSET_TEST_CASES()                                                             \
	LONE_TYPES_OFFSET_TEST_ALL(u, 16, 3)                                                       \
	LONE_TYPES_OFFSET_TEST_ALL(s, 16, 3)                                                       \
	LONE_TYPES_OFFSET_TEST_ALL(u, 32, 3)                                                       \
	LONE_TYPES_OFFSET_TEST_ALL(s, 32, 3)                                                       \
	LONE_TYPES_OFFSET_TEST_ALL(u, 64, 3)                                                       \
	LONE_TYPES_OFFSET_TEST_ALL(s, 64, 3)                                                       \
	LONE_TYPES_OFFSET_TEST_ALL(u, 64, 5)                                                       \
	LONE_TYPES_OFFSET_TEST_ALL(s, 64, 5)                                                       \
	LONE_TYPES_OFFSET_TEST_ALL(u, 64, 7)                                                       \
	LONE_TYPES_OFFSET_TEST_ALL(s, 64, 7)

		LONE_TYPES_OFFSET_TEST_CASES()

#undef LONE_TYPES_OFFSET_TEST_CASES
#undef LONE_TYPES_OFFSET_TEST_ALL
#undef LONE_TYPES_OFFSET_TEST_CASE

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
	LONE_TYPES_BYTES_TEST_CASE_2(sign,  8)                                                     \
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

		LONE_TYPES_ENDIAN_TEST_CASES()

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

#include <lone/architecture/linux/entry.c>
