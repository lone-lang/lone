/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/types.h>
#include <lone/bits.h>

#include <lone/test.h>

static LONE_TEST_FUNCTION(test_lone_bits_get)
{
	unsigned char bits[] = {
		0, /* 0  -  7 */
		0, /* 8  - 15 */
		0, /* 16 - 23 */
		0, /* 24 - 31 */
		0, /* 32 - 39 */
		0, /* 40 - 47 */
		1, /* 48 - 55 */
		0, /* 56 - 63 */
	};

	lone_test_assert_true(suite, test, lone_bits_get(bits, 55));
	lone_test_assert_false(suite, test, lone_bits_get(bits, 48));
}

static LONE_TEST_FUNCTION(test_lone_bits_set)
{
	unsigned char bits[] = {
		0, /* 0  -  7 */
		0, /* 8  - 15 */
		0, /* 16 - 23 */
		0, /* 24 - 31 */
		0, /* 32 - 39 */
		0, /* 40 - 47 */
		0, /* 48 - 55 */
		0, /* 56 - 63 */
	};

	lone_bits_set(bits, 20, 1);
	lone_bits_set(bits, 48, 1);

	lone_test_assert_unsigned_char_equal(suite, test, bits[2], 8);
	lone_test_assert_unsigned_char_equal(suite, test, bits[6], 128);
}

static LONE_TEST_FUNCTION(test_lone_bits_find_first_zero)
{
	unsigned char bits[] = {
		255, /* 0  -  7 */
		255, /* 8  - 15 */
		255, /* 16 - 23 */
		255, /* 24 - 31 */
		255, /* 32 - 39 */
		255, /* 40 - 47 */
		239, /* 48 - 55 */
		255, /* 56 - 63 */
	};

	lone_size i = lone_bits_find_first_zero(bits, sizeof(bits));
	lone_test_assert_true(suite, test, i == 51);
}

static LONE_TEST_FUNCTION(test_lone_bits_find_first_one)
{
	unsigned char bits[] = {
		0, /* 0  -  7 */
		0, /* 8  - 15 */
		0, /* 16 - 23 */
		0, /* 24 - 31 */
		4, /* 32 - 39 */
		0, /* 40 - 47 */
		0, /* 48 - 55 */
		0, /* 56 - 63 */
	};

	lone_size i = lone_bits_find_first_one(bits, sizeof(bits));
	lone_test_assert_true(suite, test, i == 37);
}

long lone(int argc, char **argv, char **envp, struct lone_auxiliary_vector *auxv)
{

	static struct lone_test_case cases[] = {

		LONE_TEST_CASE("lone/bits/get", test_lone_bits_get),
		LONE_TEST_CASE("lone/bits/set", test_lone_bits_set),
		LONE_TEST_CASE("lone/bits/find-first/zero", test_lone_bits_find_first_zero),
		LONE_TEST_CASE("lone/bits/find-first/one", test_lone_bits_find_first_one),

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
