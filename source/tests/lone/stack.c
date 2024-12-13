/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/types.h>
#include <lone/stack.h>

#include <lone/test.h>

static LONE_TEST_FUNCTION(test_lone_stack_documentation_example)
{
	unsigned char memory[16] = { 0 };
	void *base = memory, *limit = memory + sizeof(memory), *top = base;

	lone_test_assert_true(suite,  test, lone_stack_is_empty(top, base));
	lone_test_assert_false(suite, test, lone_stack_is_full(top,  limit));

	lone_test_assert_true(suite,  test, lone_stack_can_push(top, limit, sizeof(lone_u64)));
	lone_stack_push_u64(&top, 0x0102030405060708);
	lone_test_assert_false(suite, test, lone_stack_is_empty(top, base));
	lone_test_assert_false(suite, test, lone_stack_is_full(top,  limit));

	lone_test_assert_true(suite, test, lone_stack_can_push(top, limit, sizeof(lone_u32)));
	lone_stack_push_u32(&top, 0x09101112);
	lone_test_assert_false(suite, test, lone_stack_is_empty(top, base));
	lone_test_assert_false(suite, test, lone_stack_is_full(top,  limit));

	lone_test_assert_true(suite, test, lone_stack_can_push(top, limit, sizeof(lone_u16)));
	lone_stack_push_u16(&top, 0x1314);
	lone_test_assert_false(suite, test, lone_stack_is_empty(top, base));
	lone_test_assert_false(suite, test, lone_stack_is_full(top,  limit));

	lone_test_assert_true(suite, test, lone_stack_can_push(top, limit, sizeof(lone_u8)));
	lone_stack_push_u8(&top, 0x15);
	lone_test_assert_false(suite, test, lone_stack_is_empty(top, base));
	lone_test_assert_false(suite, test, lone_stack_is_full(top,  limit));

	lone_test_assert_true(suite, test, lone_stack_can_push(top, limit, sizeof(lone_u8)));
	lone_stack_push_u8(&top, 0x16);
	lone_test_assert_false(suite, test, lone_stack_is_empty(top, base));
	lone_test_assert_true(suite,  test, lone_stack_is_full(top,  limit));

	lone_test_assert_false(suite, test, lone_stack_can_push(top, limit, sizeof(lone_u8)));

	lone_test_assert_true(suite, test, lone_stack_can_pop(top, base, sizeof(lone_u8)));
	lone_test_assert_u8_equal(suite,  test, lone_stack_pop_u8 (&top), 0x16);
	lone_test_assert_false(suite, test, lone_stack_is_empty(top, base));
	lone_test_assert_false(suite, test, lone_stack_is_full(top,  limit));

	lone_test_assert_true(suite, test, lone_stack_can_pop(top, base, sizeof(lone_u8)));
	lone_test_assert_u8_equal(suite,  test, lone_stack_pop_u8 (&top), 0x15);
	lone_test_assert_false(suite, test, lone_stack_is_empty(top, base));
	lone_test_assert_false(suite, test, lone_stack_is_full(top,  limit));

	lone_test_assert_true(suite, test, lone_stack_can_pop(top, base, sizeof(lone_u16)));
	lone_test_assert_u16_equal(suite, test, lone_stack_pop_u16(&top), 0x1314);
	lone_test_assert_false(suite, test, lone_stack_is_empty(top, base));
	lone_test_assert_false(suite, test, lone_stack_is_full(top,  limit));

	lone_test_assert_true(suite, test, lone_stack_can_pop(top, base, sizeof(lone_u32)));
	lone_test_assert_u32_equal(suite, test, lone_stack_pop_u32(&top), 0x09101112);
	lone_test_assert_false(suite, test, lone_stack_is_empty(top, base));
	lone_test_assert_false(suite, test, lone_stack_is_full(top,  limit));

	lone_test_assert_true(suite, test, lone_stack_can_pop(top, base, sizeof(lone_u64)));
	lone_test_assert_u64_equal(suite, test, lone_stack_pop_u64(&top), 0x0102030405060708);
	lone_test_assert_true(suite,  test, lone_stack_is_empty(top, base));
	lone_test_assert_false(suite, test, lone_stack_is_full(top,  limit));

	lone_test_assert_false(suite, test, lone_stack_can_pop(top, base, sizeof(lone_u8)));
}

long lone(int argc, char **argv, char **envp, struct lone_auxiliary_vector *auxv)
{

	static struct lone_test_case cases[] = {

		LONE_TEST_CASE("lone/stack/documentation-example", test_lone_stack_documentation_example),

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

#include <lone/architecture/linux/entry_point.c>
