/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/test.h>
#include <lone/linux.h>

static void lone_test_result_override(enum lone_test_result *result, struct lone_test_case *test)
{
	*result = test->result;
}

static void lone_test_result_override_all_but_error(enum lone_test_result *result, struct lone_test_case *test)
{
	switch (*result) {
	case LONE_TEST_RESULT_ERROR:
		return;
	case LONE_TEST_RESULT_PENDING:
	case LONE_TEST_RESULT_PASSED:
	case LONE_TEST_RESULT_FAILED:
	case LONE_TEST_RESULT_SKIPPED:
		lone_test_result_override(result, test);
		break;
	}
}

static void lone_test_result_override_pending(enum lone_test_result *result, struct lone_test_case *test)
{
	switch (*result) {
	case LONE_TEST_RESULT_PENDING:
		lone_test_result_override(result, test);
		break;
	default:
		return;
	}
}

static void lone_test_result_override_pending_or_skipped(enum lone_test_result *result, struct lone_test_case *test)
{
	switch (*result) {
	case LONE_TEST_RESULT_PENDING:
	case LONE_TEST_RESULT_SKIPPED:
		lone_test_result_override(result, test);
		break;
	default:
		return;
	}
}

static void lone_test_event_dispatch(lone_test_event event,
		struct lone_test_suite *suite, struct lone_test_case *test)
{
	if (event) { event(suite, test); }
}

static void lone_test_suite_dispatch_event(struct lone_test_suite *suite, lone_test_event event, struct lone_test_case *test)
{
	lone_test_event_dispatch(event, suite, test);
}

static void lone_test_suite_test_started(struct lone_test_suite *suite, struct lone_test_case *test)
{
	lone_test_suite_dispatch_event(suite, suite->events.on.test.started, test);
}

static void lone_test_suite_test_finished(struct lone_test_suite *suite, struct lone_test_case *test)
{
	lone_test_suite_dispatch_event(suite, suite->events.on.test.finished, test);
}

enum lone_test_result lone_test_suite_run(struct lone_test_suite *suite)
{
	enum lone_test_result result;
	struct lone_test_case *current;

	result = LONE_TEST_RESULT_PENDING;

	if (!suite || !suite->tests) { goto end; }

	for (current = suite->tests; current->test; ++current) {
		lone_test_suite_test_started(suite, current);
		current->test(suite, current);
		lone_test_suite_test_finished(suite, current);

		switch (current->result) {
		case LONE_TEST_RESULT_SKIPPED:
			lone_test_result_override_pending(&result, current);
			break;
		case LONE_TEST_RESULT_PASSED:
			lone_test_result_override_pending_or_skipped(&result, current);
			break;
		case LONE_TEST_RESULT_FAILED:
			lone_test_result_override_all_but_error(&result, current);
			break;
		case LONE_TEST_RESULT_ERROR:
			lone_test_result_override(&result, current);
			break;
		case LONE_TEST_RESULT_PENDING:
		default:
			linux_exit(-1);
		}
	}

end:
	return result;
}

static bool lone_test_assert_equal(struct lone_test_suite *suite,
		struct lone_test_case *test, struct lone_test_assertion *assertion)
{
	switch (assertion->values_type) {
	case LONE_TYPES_C_CHAR:
		return assertion->x.as.plain_char == assertion->y.as.plain_char;
	case LONE_TYPES_C_SIGNED_CHAR:
		return assertion->x.as.signed_char == assertion->y.as.signed_char;
	case LONE_TYPES_C_UNSIGNED_CHAR:
		return assertion->x.as.unsigned_char == assertion->y.as.unsigned_char;

	case LONE_TYPES_C_SHORT:
		return assertion->x.as.signed_short == assertion->y.as.signed_short;
	case LONE_TYPES_C_UNSIGNED_SHORT:
		return assertion->x.as.unsigned_short == assertion->y.as.unsigned_short;

	case LONE_TYPES_C_INT:
		return assertion->x.as.signed_int == assertion->y.as.signed_int;
	case LONE_TYPES_C_UNSIGNED_INT:
		return assertion->x.as.unsigned_int == assertion->y.as.unsigned_int;

	case LONE_TYPES_C_LONG:
		return assertion->x.as.signed_long == assertion->y.as.signed_long;
	case LONE_TYPES_C_UNSIGNED_LONG:
		return assertion->x.as.unsigned_long == assertion->y.as.unsigned_long;

	case LONE_TYPES_C_LONG_LONG:
		return assertion->x.as.signed_long_long == assertion->y.as.signed_long_long;
	case LONE_TYPES_C_UNSIGNED_LONG_LONG:
		return assertion->x.as.unsigned_long_long == assertion->y.as.unsigned_long_long;

	case LONE_TYPES_C_BOOLEAN:
		return assertion->x.as.boolean == assertion->y.as.boolean;

	case LONE_TYPES_C_POINTER:
		return assertion->x.as.pointer == assertion->y.as.pointer;
	case LONE_TYPES_C_FUNCTION_POINTER:
		return assertion->x.as.function_pointer == assertion->y.as.function_pointer;

	case LONE_TYPES_C_S8:
		return assertion->x.as.s8  == assertion->y.as.s8;
	case LONE_TYPES_C_S16:
		return assertion->x.as.s16 == assertion->y.as.s16;
	case LONE_TYPES_C_S32:
		return assertion->x.as.s32 == assertion->y.as.s32;
	case LONE_TYPES_C_S64:
		return assertion->x.as.s64 == assertion->y.as.s64;

	case LONE_TYPES_C_U8:
		return assertion->x.as.u8  == assertion->y.as.u8;
	case LONE_TYPES_C_U16:
		return assertion->x.as.u16 == assertion->y.as.u16;
	case LONE_TYPES_C_U32:
		return assertion->x.as.u32 == assertion->y.as.u32;
	case LONE_TYPES_C_U64:
		return assertion->x.as.u64 == assertion->y.as.u64;

	case LONE_TYPES_C_UNDEFINED:
	default:
		test->result = LONE_TEST_RESULT_ERROR;
		return false;
	}
}

bool lone_test_assert(struct lone_test_suite *suite,
		struct lone_test_case *test, struct lone_test_assertion *assertion)
{
	bool result;

	switch (assertion->type) {
	case LONE_TEST_ASSERTION_TYPE_EQUAL:
		result = lone_test_assert_equal(suite, test, assertion);
		break;
	case LONE_TEST_ASSERTION_TYPE_NOT_EQUAL:
		result = !lone_test_assert_equal(suite, test, assertion);
		break;
	}

	if (test->result == LONE_TEST_RESULT_PENDING) {
		test->result = result? LONE_TEST_RESULT_PASSED : LONE_TEST_RESULT_FAILED;
	}

	return result;
}

bool lone_test_assert_true(struct lone_test_suite *suite,
		struct lone_test_case *test, bool b)
{
	return lone_test_assert_boolean_equal(suite, test, b, true);
}

bool lone_test_assert_false(struct lone_test_suite *suite,
		struct lone_test_case *test, bool b)
{
	return lone_test_assert_boolean_equal(suite, test, b, false);
}

#define LONE_TEST_ASSERTION_FUNCTION_5(name, params_type, assertion_type, value_type, field)       \
bool lone_test_assert_##name(struct lone_test_suite *suite, struct lone_test_case *test,           \
		params_type x, params_type y)                                                      \
{                                                                                                  \
	struct lone_test_assertion assertion = {                                                   \
		.type = LONE_TEST_ASSERTION_TYPE_##assertion_type,                                 \
		.values_type = LONE_TYPES_C_##value_type,                                          \
		.x.as.field = x,                                                                   \
		.y.as.field = y,                                                                   \
	};                                                                                         \
                                                                                                   \
	return lone_test_assert(suite, test, &assertion);                                          \
}

#define LONE_TEST_ASSERTION_FUNCTION(name, params_type, value_type, field)                         \
LONE_TEST_ASSERTION_FUNCTION_5(name##_equal,     params_type, EQUAL,     value_type, field)        \
LONE_TEST_ASSERTION_FUNCTION_5(name##_not_equal, params_type, NOT_EQUAL, value_type, field)

LONE_TEST_ASSERTION_FUNCTION(boolean, bool, BOOLEAN, boolean)

LONE_TEST_ASSERTION_FUNCTION(char,                   char,    PLAIN_CHAR,    plain_char)
LONE_TEST_ASSERTION_FUNCTION(signed_char,     signed char,   SIGNED_CHAR,   signed_char)
LONE_TEST_ASSERTION_FUNCTION(unsigned_char, unsigned char, UNSIGNED_CHAR, unsigned_char)

LONE_TEST_ASSERTION_FUNCTION(short,            signed short,   SIGNED_SHORT,   signed_short)
LONE_TEST_ASSERTION_FUNCTION(unsigned_short, unsigned short, UNSIGNED_SHORT, unsigned_short)

LONE_TEST_ASSERTION_FUNCTION(int,            signed int,   SIGNED_INT,   signed_int)
LONE_TEST_ASSERTION_FUNCTION(unsigned_int, unsigned int, UNSIGNED_INT, unsigned_int)

LONE_TEST_ASSERTION_FUNCTION(long,           signed long,   SIGNED_LONG,   signed_long)
LONE_TEST_ASSERTION_FUNCTION(unsigned_long, unsigned long, UNSIGNED_LONG, unsigned_long)

LONE_TEST_ASSERTION_FUNCTION(long_long,            signed long long,   SIGNED_LONG_LONG,   signed_long_long)
LONE_TEST_ASSERTION_FUNCTION(unsigned_long_long, unsigned long long, UNSIGNED_LONG_LONG, unsigned_long_long)

LONE_TEST_ASSERTION_FUNCTION(s8,  lone_s8,  S8,  s8)
LONE_TEST_ASSERTION_FUNCTION(u8,  lone_u8,  U8,  u8)
LONE_TEST_ASSERTION_FUNCTION(s16, lone_s16, S16, s16)
LONE_TEST_ASSERTION_FUNCTION(u16, lone_u16, U16, u16)
LONE_TEST_ASSERTION_FUNCTION(s32, lone_s32, S32, s32)
LONE_TEST_ASSERTION_FUNCTION(u32, lone_u32, U32, u32)
LONE_TEST_ASSERTION_FUNCTION(s64, lone_s64, S64, s64)
LONE_TEST_ASSERTION_FUNCTION(u64, lone_u64, U64, u64)

#undef LONE_TEST_ASSERTION_FUNCTION
#undef LONE_TEST_ASSERTION_FUNCTION_5
