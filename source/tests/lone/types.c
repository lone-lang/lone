/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/types.h>
#include <lone/memory/functions.h>
#include <lone/test.h>
#include <lone/linux.h>

static enum lone_test_result test_lone_types_lone_u8_read(void *context)
{
	lone_u8 u8 = 214;

	return
		(lone_u8_read(&u8) == 214) ?
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;
}

static enum lone_test_result test_lone_types_lone_s8_read(void *context)
{
	lone_s8 s8 = -42;

	return
		(lone_s8_read(&s8) == -42) ?
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;
}

static enum lone_test_result test_lone_types_lone_u16_read(void *context)
{
	lone_u16 u16 = 56535;

	return
		(lone_u16_read(&u16) == 56535) ?
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;
}

static enum lone_test_result test_lone_types_lone_s16_read(void *context)
{
	lone_s16 s16 = -9001;

	return
		(lone_s16_read(&s16) == -9001) ?
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;
}

static enum lone_test_result test_lone_types_lone_u32_read(void *context)
{
	lone_u32 u32 = 4294927296;

	return
		(lone_u32_read(&u32) == 4294927296) ?
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;
}

static enum lone_test_result test_lone_types_lone_s32_read(void *context)
{
	lone_s32 s32 = -40000;

	return
		(lone_s32_read(&s32) == -40000) ?
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;
}

static enum lone_test_result test_lone_types_lone_u64_read(void *context)
{
	lone_u64 u64 = 17212176182698430302UL;

	return
		(lone_u64_read(&u64) == 17212176182698430302UL) ?
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;
}

static enum lone_test_result test_lone_types_lone_s64_read(void *context)
{
	lone_s64 s64 = -1234567891011121314;

	return
		(lone_s64_read(&s64) == -1234567891011121314) ?
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;
}

static enum lone_test_result test_lone_types_lone_u8_write(void *context)
{
	lone_u8 u8 = 0;

	lone_u8_write(&u8, 214);

	return u8 == 214 ?
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;
}

static enum lone_test_result test_lone_types_lone_s8_write(void *context)
{
	lone_s8 s8 = 0;

	lone_s8_write(&s8, -42);

	return s8 == -42 ?
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;
}

static enum lone_test_result test_lone_types_lone_u16_write(void *context)
{
	lone_u16 u16 = 0;

	lone_u16_write(&u16, 56535);

	return u16 == 56535 ?
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;
}

static enum lone_test_result test_lone_types_lone_s16_write(void *context)
{
	lone_s16 s16 = 0;

	lone_s16_write(&s16, -9001);

	return s16 == -9001 ?
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;
}

static enum lone_test_result test_lone_types_lone_u32_write(void *context)
{
	lone_u32 u32 = 0;

	lone_u32_write(&u32, 4294927296);

	return u32 == 4294927296 ?
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;
}

static enum lone_test_result test_lone_types_lone_s32_write(void *context)
{
	lone_s32 s32 = 0;

	lone_s32_write(&s32, -40000);

	return s32 == -40000 ?
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;
}

static enum lone_test_result test_lone_types_lone_u64_write(void *context)
{
	lone_u64 u64 = 0;

	lone_u64_write(&u64, 17212176182698430302UL);

	return u64 == 17212176182698430302UL ?
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;
}

static enum lone_test_result test_lone_types_lone_s64_write(void *context)
{
	lone_s64 s64 = 0;

	lone_s64_write(&s64, -1234567891011121314);

	return s64 == -1234567891011121314 ?
		LONE_TEST_RESULT_PASS : LONE_TEST_RESULT_FAIL;
}

static void test_finished(struct lone_test_case *test, void *context)
{
	struct lone_bytes result;

	switch (test->result) {
	case LONE_TEST_RESULT_PASS:
		result = LONE_BYTES_FROM_LITERAL("PASS");
		break;
	case LONE_TEST_RESULT_FAIL:
		result = LONE_BYTES_FROM_LITERAL("FAIL");
		break;
	case LONE_TEST_RESULT_SKIP:
		result = LONE_BYTES_FROM_LITERAL("SKIP");
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

		LONE_TEST_CASE("lone/types/u8/read",   test_lone_types_lone_u8_read),
		LONE_TEST_CASE("lone/types/s8/read",   test_lone_types_lone_s8_read),
		LONE_TEST_CASE("lone/types/u16/read",  test_lone_types_lone_u16_read),
		LONE_TEST_CASE("lone/types/s16/read",  test_lone_types_lone_s16_read),
		LONE_TEST_CASE("lone/types/u32/read",  test_lone_types_lone_u32_read),
		LONE_TEST_CASE("lone/types/s32/read",  test_lone_types_lone_s32_read),
		LONE_TEST_CASE("lone/types/u64/read",  test_lone_types_lone_u64_read),
		LONE_TEST_CASE("lone/types/s64/read",  test_lone_types_lone_s64_read),

		LONE_TEST_CASE("lone/types/u8/write",   test_lone_types_lone_u8_write),
		LONE_TEST_CASE("lone/types/s8/write",   test_lone_types_lone_s8_write),
		LONE_TEST_CASE("lone/types/u16/write",  test_lone_types_lone_u16_write),
		LONE_TEST_CASE("lone/types/s16/write",  test_lone_types_lone_s16_write),
		LONE_TEST_CASE("lone/types/u32/write",  test_lone_types_lone_u32_write),
		LONE_TEST_CASE("lone/types/s32/write",  test_lone_types_lone_s32_write),
		LONE_TEST_CASE("lone/types/u64/write",  test_lone_types_lone_u64_write),
		LONE_TEST_CASE("lone/types/s64/write",  test_lone_types_lone_s64_write),

		{0},
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
