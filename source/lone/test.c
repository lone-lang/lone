/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/test.h>
#include <lone/linux.h>

static void lone_test_result_override(enum lone_test_result *result, struct lone_test_case *test)
{
	*result = test->result;
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
	case LONE_TEST_RESULT_PENDING:     __attribute__((fallthrough));
	case LONE_TEST_RESULT_SKIPPED:
		lone_test_result_override(result, test);
		break;
	default:
		return;
	}
}

static void lone_test_event_dispatch(lone_test_event event, struct lone_test_case *test, void *context)
{
	if (event) { event(test, context); }
}

static void lone_test_suite_dispatch_event(struct lone_test_suite *suite, lone_test_event event, struct lone_test_case *test)
{
	lone_test_event_dispatch(event, test, suite->events.context);
}

static void lone_test_suite_test_initiated(struct lone_test_suite *suite, struct lone_test_case *test)
{
	lone_test_suite_dispatch_event(suite, suite->events.on.test.initiated, test);
}

static void lone_test_suite_test_terminated(struct lone_test_suite *suite, struct lone_test_case *test)
{
	lone_test_suite_dispatch_event(suite, suite->events.on.test.terminated, test);
}

enum lone_test_result lone_test_suite_run(struct lone_test_suite *suite)
{
	enum lone_test_result result;
	struct lone_test_case *current;

	result = LONE_TEST_RESULT_PENDING;

	if (!suite || !suite->tests) { goto end; }

	for (current = suite->tests; current->test; ++current) {
		lone_test_suite_test_initiated(suite, current);
		current->result = current->test(current->context);
		lone_test_suite_test_terminated(suite, current);

		switch (current->result) {
		case LONE_TEST_RESULT_SKIPPED:
			lone_test_result_override_pending(&result, current);
			break;
		case LONE_TEST_RESULT_PASSED:
			lone_test_result_override_pending_or_skipped(&result, current);
			break;
		case LONE_TEST_RESULT_FAILED:
			lone_test_result_override(&result, current);
			break;
		case LONE_TEST_RESULT_PENDING:    __attribute__((fallthrough));
		default:
			linux_exit(-1);
		}
	}

end:
	return result;
}