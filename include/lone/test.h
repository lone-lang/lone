/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_TEST_HEADER
#define LONE_TEST_HEADER

#include <lone/types.h>

struct lone_test_suite;
struct lone_test_case;
struct lone_test_assertion;

enum lone_test_result {
	LONE_TEST_RESULT_PENDING   = 0, /* initial state, it is an error for a test case to return this */

	LONE_TEST_RESULT_PASSED    = 1, /* test executed successfully, results were correct and expected */
	LONE_TEST_RESULT_FAILED    = 2, /* test executed successfully, results were incorrect or unexpected */
	LONE_TEST_RESULT_SKIPPED   = 3, /* test couldn't execute due to non-error condition and was skipped */

	LONE_TEST_RESULT_ERROR     = 4, /* unexpected error was encountered during test execution */

	LONE_TEST_RESULT_PASS      = LONE_TEST_RESULT_PASSED,
	LONE_TEST_RESULT_FAIL      = LONE_TEST_RESULT_FAILED,
	LONE_TEST_RESULT_SKIP      = LONE_TEST_RESULT_SKIPPED,
};

enum lone_test_assertion_type {
	LONE_TEST_ASSERTION_TYPE_EQUAL,
	LONE_TEST_ASSERTION_TYPE_NOT_EQUAL,
};

typedef void (*lone_test_function)(struct lone_test_suite *suite,
		struct lone_test_case *test);
typedef void (*lone_test_event)(struct lone_test_suite *suite,
		struct lone_test_case *test);

struct lone_test_suite_events {
	void *context;

	struct {
		struct {
			lone_test_event started;
			lone_test_event finished;
		} test;
	} on;
};

struct lone_test_suite {
	struct lone_test_case *tests;
	struct lone_test_suite_events events;
};

struct lone_test_case {
	struct lone_bytes name;
	enum lone_test_result result;
	lone_test_function test;
	void *context;
};

struct lone_test_assertion {
	enum lone_test_assertion_type type;

	enum   lone_c_type   values_type;
	struct lone_c_values x;
	struct lone_c_values y;
};

#define LONE_TEST_FUNCTION(__name)                                                                 \
void __name(struct lone_test_suite *suite, struct lone_test_case *test)

#define LONE_TEST_CASE(__name_c_string_literal, __test) \
	{ \
		.name = LONE_BYTES_FROM_LITERAL(__name_c_string_literal), \
		.test = (__test), \
		.context = 0, \
		.result = LONE_TEST_RESULT_PENDING \
	}

#define LONE_TEST_CASE_WITH_CONTEXT(__name_c_string_literal, __test, __context) \
	{ \
		.name = LONE_BYTES_FROM_LITERAL(__name_c_string_literal), \
		.test = (__test), \
		.context = (__context), \
		.result = LONE_TEST_RESULT_PENDING \
	}

#define LONE_TEST_CASE_NULL() \
	{ \
		.name = LONE_BYTES_NULL(), \
		.test = 0, \
		.context = 0, \
		.result = LONE_TEST_RESULT_PENDING \
	}

#define LONE_TEST_SUITE(__cases) \
	{ \
		.tests = (__cases), \
		.events.context = 0, \
		.events.on.test.started = 0, \
		.events.on.test.finished = 0, \
	}

enum lone_test_result lone_test_suite_run(struct lone_test_suite *suite);

bool lone_test_assert(struct lone_test_suite *suite,
		struct lone_test_case *test, struct lone_test_assertion *assertion);

bool lone_test_assert_true(struct lone_test_suite *suite,
		struct lone_test_case *test, bool b);

bool lone_test_assert_false(struct lone_test_suite *suite,
		struct lone_test_case *test, bool b);

bool lone_test_assert_boolean_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, bool x, bool y);

bool lone_test_assert_boolean_not_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, bool x, bool y);

bool lone_test_assert_char_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, char x, char y);

bool lone_test_assert_char_not_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, char x, char y);

bool lone_test_assert_signed_char_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, signed char x, signed char y);

bool lone_test_assert_signed_char_not_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, signed char x, signed char y);

bool lone_test_assert_unsigned_char_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, unsigned char x, unsigned char y);

bool lone_test_assert_unsigned_char_not_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, unsigned char x, unsigned char y);

bool lone_test_assert_short_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, signed short x, signed short y);

bool lone_test_assert_short_not_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, signed short x, signed short y);

bool lone_test_assert_unsigned_short_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, unsigned short x, unsigned short y);

bool lone_test_assert_unsigned_short_not_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, unsigned short x, unsigned short y);

bool lone_test_assert_int_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, signed int x, signed int y);

bool lone_test_assert_int_not_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, signed int x, signed int y);

bool lone_test_assert_unsigned_int_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, unsigned int x, unsigned int y);

bool lone_test_assert_unsigned_int_not_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, unsigned int x, unsigned int y);

bool lone_test_assert_long_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, signed long x, signed long y);

bool lone_test_assert_long_not_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, signed long x, signed long y);

bool lone_test_assert_unsigned_long_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, unsigned long x, unsigned long y);

bool lone_test_assert_unsigned_long_not_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, unsigned long x, unsigned long y);

bool lone_test_assert_long_long_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, signed long long x, signed long long y);

bool lone_test_assert_long_long_not_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, signed long long x, signed long long y);

bool lone_test_assert_unsigned_long_long_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, unsigned long long x, unsigned long long y);

bool lone_test_assert_unsigned_long_long_not_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, unsigned long long x, unsigned long long y);

bool lone_test_assert_s8_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, lone_s8 x, lone_s8 y);

bool lone_test_assert_s8_not_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, lone_s8 x, lone_s8 y);

bool lone_test_assert_u8_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, lone_u8 x, lone_u8 y);

bool lone_test_assert_u8_not_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, lone_u8 x, lone_u8 y);

bool lone_test_assert_s16_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, lone_s16 x, lone_s16 y);

bool lone_test_assert_s16_not_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, lone_s16 x, lone_s16 y);

bool lone_test_assert_u16_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, lone_u16 x, lone_u16 y);

bool lone_test_assert_u16_not_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, lone_u16 x, lone_u16 y);

bool lone_test_assert_s32_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, lone_s32 x, lone_s32 y);

bool lone_test_assert_s32_not_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, lone_s32 x, lone_s32 y);

bool lone_test_assert_u32_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, lone_u32 x, lone_u32 y);

bool lone_test_assert_u32_not_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, lone_u32 x, lone_u32 y);

bool lone_test_assert_s64_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, lone_s64 x, lone_s64 y);

bool lone_test_assert_s64_not_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, lone_s64 x, lone_s64 y);

bool lone_test_assert_u64_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, lone_u64 x, lone_u64 y);

bool lone_test_assert_u64_not_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, lone_u64 x, lone_u64 y);

#endif /* LONE_TEST_HEADER */
