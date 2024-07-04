/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/types.h>
#include <lone/memory/functions.h>
#include <lone/test.h>
#include <lone/linux.h>

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
