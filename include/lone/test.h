/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_TEST_HEADER
#define LONE_TEST_HEADER

#include <lone/types.h>

struct lone_test_case;

enum lone_test_result {
	LONE_TEST_RESULT_PENDING   = 0, /* initial state, it is an error for a test case to return this */

	LONE_TEST_RESULT_PASSED    = 1, /* test executed successfully, results were correct and expected */
	LONE_TEST_RESULT_FAILED    = 2, /* test executed successfully, results were incorrect or unexpected */
	LONE_TEST_RESULT_SKIPPED   = 3, /* test couldn't execute due to non-error condition and was skipped */

	LONE_TEST_RESULT_PASS      = LONE_TEST_RESULT_PASSED,
	LONE_TEST_RESULT_FAIL      = LONE_TEST_RESULT_FAILED,
	LONE_TEST_RESULT_SKIP      = LONE_TEST_RESULT_SKIPPED,
};

typedef enum lone_test_result (*lone_test_function)(void *context);
typedef void (*lone_test_event)(struct lone_test_case *test, void *context);

struct lone_test_suite_events {
	void *context;

	struct {
		struct {
			lone_test_event initiated;
			lone_test_event terminated;
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

#define LONE_TEST_CASE_WITH_CONTEXT(name_c_string_literal, test_function, _context)       \
	{                                                                                 \
		.name    = LONE_BYTES_FROM_LITERAL((name_c_string_literal)),              \
		.test    = (test_function),                                               \
		.context = (_context),                                                    \
		.result  = LONE_TEST_RESULT_PENDING,                                      \
	}

#define LONE_TEST_CASE(name_c_string_literal, test_function) \
	LONE_TEST_CASE_WITH_CONTEXT((name_c_string_literal), (test_function), 0)

#define LONE_TEST_SUITE(cases)                                                            \
	{                                                                                 \
		.tests                     = (cases),                                     \
		.events.context            = 0,                                           \
		.events.on.test.initiated  = 0,                                           \
		.events.on.test.terminated = 0,                                           \
	}

enum lone_test_result lone_test_suite_run(struct lone_test_suite *suite);

#endif /* LONE_TEST_HEADER */

