/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/types.h>
#include <lone/memory/functions.h>
#include <lone/test.h>
#include <lone/linux.h>

#define LONE_TYPES_TEST_NAME(type, operation, alignment) \
	"lone/types/" #type "/" #operation "/" #alignment

#define LONE_TYPES_TEST_FUNCTION(type, operation, alignment) \
	test_lone_types_lone_##type##_##operation##_##alignment

#define LONE_TYPES_TEST_READ_ALIGNED(type, constant)                                               \
static enum lone_test_result LONE_TYPES_TEST_FUNCTION(type, read, aligned)(void *context)          \
{                                                                                                  \
	lone_##type value = constant;                                                              \
	                                                                                           \
	return                                                                                     \
		(lone_##type##_read(&value) == constant) ?                                         \
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;                                     \
}

#define LONE_TYPES_TEST_READ_UNALIGNED(type, constant)                                             \
static enum lone_test_result LONE_TYPES_TEST_FUNCTION(type, read, unaligned)(void *context)        \
{                                                                                                  \
	struct __attribute__((packed)) {                                                           \
		unsigned char byte;                                                                \
		lone_##type value;                                                                 \
	} unaligned = { 0, constant };                                                             \
	                                                                                           \
	return                                                                                     \
		(lone_##type##_read(&unaligned.value) == constant) ?                               \
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;                                     \
}

#define LONE_TYPES_TEST_WRITE_ALIGNED(type, constant)                                              \
static enum lone_test_result LONE_TYPES_TEST_FUNCTION(type, write, aligned)(void *context)         \
{                                                                                                  \
	lone_##type value = 0;                                                                     \
	                                                                                           \
	lone_##type##_write(&value, constant);                                                     \
	                                                                                           \
	return value == constant ?                                                                 \
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;                                     \
}

#define LONE_TYPES_TEST_WRITE_UNALIGNED(type, constant)                                            \
static enum lone_test_result LONE_TYPES_TEST_FUNCTION(type, write, unaligned)(void *context)       \
{                                                                                                  \
	struct __attribute__((packed)) {                                                           \
		unsigned char byte;                                                                \
		lone_##type value;                                                                 \
	} unaligned = {0};                                                                         \
	                                                                                           \
	lone_##type##_write(&unaligned.value, constant);                                           \
	                                                                                           \
	return unaligned.value == constant ?                                                       \
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;                                     \
}

LONE_TYPES_TEST_READ_ALIGNED(u8,  +214)
LONE_TYPES_TEST_READ_ALIGNED(s8,  -42)
LONE_TYPES_TEST_READ_ALIGNED(u16, +56535)
LONE_TYPES_TEST_READ_ALIGNED(s16, -9001)
LONE_TYPES_TEST_READ_ALIGNED(u32, +4294927296)
LONE_TYPES_TEST_READ_ALIGNED(s32, -40000)
LONE_TYPES_TEST_READ_ALIGNED(u64, +17212176182698430302UL)
LONE_TYPES_TEST_READ_ALIGNED(s64, -1234567891011121314)

LONE_TYPES_TEST_READ_UNALIGNED(u16, +56535)
LONE_TYPES_TEST_READ_UNALIGNED(s16, -9001)
LONE_TYPES_TEST_READ_UNALIGNED(u32, +4294927296)
LONE_TYPES_TEST_READ_UNALIGNED(s32, -40000)
LONE_TYPES_TEST_READ_UNALIGNED(u64, +17212176182698430302UL)
LONE_TYPES_TEST_READ_UNALIGNED(s64, -1234567891011121314)

LONE_TYPES_TEST_WRITE_ALIGNED(u8,  +214)
LONE_TYPES_TEST_WRITE_ALIGNED(s8,  -42)
LONE_TYPES_TEST_WRITE_ALIGNED(u16, +56535)
LONE_TYPES_TEST_WRITE_ALIGNED(s16, -9001)
LONE_TYPES_TEST_WRITE_ALIGNED(u32, +4294927296)
LONE_TYPES_TEST_WRITE_ALIGNED(s32, -40000)
LONE_TYPES_TEST_WRITE_ALIGNED(u64, +17212176182698430302UL)
LONE_TYPES_TEST_WRITE_ALIGNED(s64, -1234567891011121314)

LONE_TYPES_TEST_WRITE_UNALIGNED(u16, +56535)
LONE_TYPES_TEST_WRITE_UNALIGNED(s16, -9001)
LONE_TYPES_TEST_WRITE_UNALIGNED(u32, +4294927296)
LONE_TYPES_TEST_WRITE_UNALIGNED(s32, -40000)
LONE_TYPES_TEST_WRITE_UNALIGNED(u64, +17212176182698430302UL)
LONE_TYPES_TEST_WRITE_UNALIGNED(s64, -1234567891011121314)

#undef LONE_TYPES_TEST_READ_ALIGNED
#undef LONE_TYPES_TEST_READ_UNALIGNED
#undef LONE_TYPES_TEST_WRITE_ALIGNED
#undef LONE_TYPES_TEST_WRITE_UNALIGNED

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

#define LONE_TYPES_TEST_CASE(type, operation, alignment)                                           \
	LONE_TEST_CASE(LONE_TYPES_TEST_NAME(type, operation, alignment),                           \
		LONE_TYPES_TEST_FUNCTION(type, operation, alignment))

	static struct lone_test_case cases[] = {

		LONE_TYPES_TEST_CASE(u8,  read, aligned),
		LONE_TYPES_TEST_CASE(s8,  read, aligned),
		LONE_TYPES_TEST_CASE(u16, read, aligned),
		LONE_TYPES_TEST_CASE(s16, read, aligned),
		LONE_TYPES_TEST_CASE(u32, read, aligned),
		LONE_TYPES_TEST_CASE(s32, read, aligned),
		LONE_TYPES_TEST_CASE(u64, read, aligned),
		LONE_TYPES_TEST_CASE(s64, read, aligned),

		LONE_TYPES_TEST_CASE(u16, read, unaligned),
		LONE_TYPES_TEST_CASE(s16, read, unaligned),
		LONE_TYPES_TEST_CASE(u32, read, unaligned),
		LONE_TYPES_TEST_CASE(s32, read, unaligned),
		LONE_TYPES_TEST_CASE(u64, read, unaligned),
		LONE_TYPES_TEST_CASE(s64, read, unaligned),

		LONE_TYPES_TEST_CASE(u8,  write, aligned),
		LONE_TYPES_TEST_CASE(s8,  write, aligned),
		LONE_TYPES_TEST_CASE(u16, write, aligned),
		LONE_TYPES_TEST_CASE(s16, write, aligned),
		LONE_TYPES_TEST_CASE(u32, write, aligned),
		LONE_TYPES_TEST_CASE(s32, write, aligned),
		LONE_TYPES_TEST_CASE(u64, write, aligned),
		LONE_TYPES_TEST_CASE(s64, write, aligned),

		LONE_TYPES_TEST_CASE(u16, write, unaligned),
		LONE_TYPES_TEST_CASE(s16, write, unaligned),
		LONE_TYPES_TEST_CASE(u32, write, unaligned),
		LONE_TYPES_TEST_CASE(s32, write, unaligned),
		LONE_TYPES_TEST_CASE(u64, write, unaligned),
		LONE_TYPES_TEST_CASE(s64, write, unaligned),

		LONE_TEST_CASE_NULL(),
	};

	struct lone_test_suite suite = LONE_TEST_SUITE(cases);
	enum lone_test_result result;

	if (argc > 1) {
		suite.events.on.test.terminated = test_finished;
	}

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

#include <lone/architecture/linux/entry_point.c>
