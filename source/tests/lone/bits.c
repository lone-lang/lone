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

	lone_test_assert_true(suite,  test, lone_bits_get(bits, 55));
	lone_test_assert_false(suite, test, lone_bits_get(bits, 48));
}

static LONE_TEST_FUNCTION(test_lone_bits_get_first_bit)
{
	unsigned char bits[] = { 128 }; /* 0b10000000 */

	lone_test_assert_true(suite,  test, lone_bits_get(bits, 0));
	lone_test_assert_false(suite, test, lone_bits_get(bits, 1));
}

static LONE_TEST_FUNCTION(test_lone_bits_get_last_bit)
{
	unsigned char bits[] = { 1 }; /* 0b00000001 */

	lone_test_assert_false(suite, test, lone_bits_get(bits, 0));
	lone_test_assert_true(suite,  test, lone_bits_get(bits, 7));
}

static LONE_TEST_FUNCTION(test_lone_bits_get_byte_boundary)
{
	unsigned char bits[] = { 1, 128 }; /* byte 0 bit 7, byte 1 bit 0 */

	lone_test_assert_true(suite,  test, lone_bits_get(bits, 7));
	lone_test_assert_true(suite,  test, lone_bits_get(bits, 8));
	lone_test_assert_false(suite, test, lone_bits_get(bits, 6));
	lone_test_assert_false(suite, test, lone_bits_get(bits, 9));
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

static LONE_TEST_FUNCTION(test_lone_bits_set_and_clear)
{
	unsigned char bits[] = { 0 };

	lone_bits_set(bits, 3, true);
	lone_test_assert_true(suite, test, lone_bits_get(bits, 3));

	lone_bits_set(bits, 3, false);
	lone_test_assert_false(suite, test, lone_bits_get(bits, 3));
	lone_test_assert_unsigned_char_equal(suite, test, bits[0], 0);
}

static LONE_TEST_FUNCTION(test_lone_bits_mark)
{
	unsigned char bits[] = { 0, 0 };

	lone_bits_mark(bits, 0);
	lone_bits_mark(bits, 15);

	lone_test_assert_unsigned_char_equal(suite, test, bits[0], 128);
	lone_test_assert_unsigned_char_equal(suite, test, bits[1], 1);
}

static LONE_TEST_FUNCTION(test_lone_bits_clear)
{
	unsigned char bits[] = { 255, 255 };

	lone_bits_clear(bits, 0);
	lone_bits_clear(bits, 15);

	lone_test_assert_unsigned_char_equal(suite, test, bits[0], 127);
	lone_test_assert_unsigned_char_equal(suite, test, bits[1], 254);
}

static LONE_TEST_FUNCTION(test_lone_bits_mark_clear_round_trip)
{
	unsigned char bits[] = { 0, 0, 0, 0 };

	lone_bits_mark(bits, 13);
	lone_test_assert_true(suite, test, lone_bits_get(bits, 13));

	lone_bits_clear(bits, 13);
	lone_test_assert_false(suite, test, lone_bits_get(bits, 13));

	/* test that neighboring bits were not disturbed */
	lone_test_assert_false(suite, test, lone_bits_get(bits, 12));
	lone_test_assert_false(suite, test, lone_bits_get(bits, 14));
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

	struct lone_optional_size result = lone_bits_find_first_one(bits, sizeof(bits));
	lone_test_assert_true(suite, test, result.present);
	lone_test_assert_unsigned_long_equal(suite, test, result.value, 37);
}

static LONE_TEST_FUNCTION(test_lone_bits_find_first_one_leading)
{
	unsigned char bits[] = { 128, 0, 0, 0, 0, 0, 0, 0 };

	struct lone_optional_size result = lone_bits_find_first_one(bits, sizeof(bits));
	lone_test_assert_true(suite, test, result.present);
	lone_test_assert_unsigned_long_equal(suite, test, result.value, 0);
}

static LONE_TEST_FUNCTION(test_lone_bits_find_first_one_trailing)
{
	unsigned char bits[] = { 0, 0, 0, 0, 0, 0, 0, 1 };

	struct lone_optional_size result = lone_bits_find_first_one(bits, sizeof(bits));
	lone_test_assert_true(suite, test, result.present);
	lone_test_assert_unsigned_long_equal(suite, test, result.value, 63);
}

static LONE_TEST_FUNCTION(test_lone_bits_find_first_one_absent)
{
	unsigned char bits[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

	struct lone_optional_size result = lone_bits_find_first_one(bits, sizeof(bits));
	lone_test_assert_false(suite, test, result.present);
}

static LONE_TEST_FUNCTION(test_lone_bits_find_first_one_word_plus_trailing)
{
	unsigned char bits[] = { 0, 0, 0, 0, 0, 0, 0, 0, 32 };

	struct lone_optional_size result = lone_bits_find_first_one(bits, sizeof(bits));
	lone_test_assert_true(suite, test, result.present);
	lone_test_assert_unsigned_long_equal(suite, test, result.value, 66);
}

static LONE_TEST_FUNCTION(test_lone_bits_find_first_one_small)
{
	unsigned char bits[] = { 0, 0, 2 };

	struct lone_optional_size result = lone_bits_find_first_one(bits, sizeof(bits));
	lone_test_assert_true(suite, test, result.present);
	lone_test_assert_unsigned_long_equal(suite, test, result.value, 22);
}

static LONE_TEST_FUNCTION(test_lone_bits_find_first_one_second_word)
{
	unsigned char bits[] = {
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 64,
	};

	struct lone_optional_size result = lone_bits_find_first_one(bits, sizeof(bits));
	lone_test_assert_true(suite, test, result.present);
	lone_test_assert_unsigned_long_equal(suite, test, result.value, 121);
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

	struct lone_optional_size result = lone_bits_find_first_zero(bits, sizeof(bits));
	lone_test_assert_true(suite, test, result.present);
	lone_test_assert_unsigned_long_equal(suite, test, result.value, 51);
}

static LONE_TEST_FUNCTION(test_lone_bits_find_first_zero_leading)
{
	unsigned char bits[] = { 127, 255, 255, 255, 255, 255, 255, 255 };

	struct lone_optional_size result = lone_bits_find_first_zero(bits, sizeof(bits));
	lone_test_assert_true(suite, test, result.present);
	lone_test_assert_unsigned_long_equal(suite, test, result.value, 0);
}

static LONE_TEST_FUNCTION(test_lone_bits_find_first_zero_trailing)
{
	unsigned char bits[] = { 255, 255, 255, 255, 255, 255, 255, 254 };

	struct lone_optional_size result = lone_bits_find_first_zero(bits, sizeof(bits));
	lone_test_assert_true(suite, test, result.present);
	lone_test_assert_unsigned_long_equal(suite, test, result.value, 63);
}

static LONE_TEST_FUNCTION(test_lone_bits_find_first_zero_absent)
{
	unsigned char bits[] = { 255, 255, 255, 255, 255, 255, 255, 255 };

	struct lone_optional_size result = lone_bits_find_first_zero(bits, sizeof(bits));
	lone_test_assert_false(suite, test, result.present);
}

static LONE_TEST_FUNCTION(test_lone_bits_find_first_zero_word_plus_trailing)
{
	unsigned char bits[] = { 255, 255, 255, 255, 255, 255, 255, 255, 223 };

	struct lone_optional_size result = lone_bits_find_first_zero(bits, sizeof(bits));
	lone_test_assert_true(suite, test, result.present);
	lone_test_assert_unsigned_long_equal(suite, test, result.value, 66);
}

static LONE_TEST_FUNCTION(test_lone_bits_find_first_zero_small)
{
	unsigned char bits[] = { 255, 255, 253 };

	struct lone_optional_size result = lone_bits_find_first_zero(bits, sizeof(bits));
	lone_test_assert_true(suite, test, result.present);
	lone_test_assert_unsigned_long_equal(suite, test, result.value, 22);
}

static LONE_TEST_FUNCTION(test_lone_bits_find_first_zero_second_word)
{
	unsigned char bits[] = {
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 191,
	};

	struct lone_optional_size result = lone_bits_find_first_zero(bits, sizeof(bits));
	lone_test_assert_true(suite, test, result.present);
	lone_test_assert_unsigned_long_equal(suite, test, result.value, 121);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    The buffer is forced to word alignment so that buffer + 3           │
   │    is guaranteed to be misaligned.                                     │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

static LONE_TEST_FUNCTION(test_lone_bits_find_first_one_unaligned_leading)
{
	unsigned char buffer[24] __attribute__((aligned(sizeof(unsigned long)))) = {
		/* [0..2] padding before bitmap */
		0, 0, 0,
		/* [3..15] bitmap: 13 bytes passed to find function */
		0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		/* [16..23] unused */
		0, 0, 0, 0, 0, 0, 0, 0,
	};

	/* bitmap byte 1 = 16 = 0b00010000, first one at bit 3 within byte */
	/* expected bit index: 1 * 8 + 3 = 11 */
	struct lone_optional_size result = lone_bits_find_first_one(buffer + 3, 13);
	lone_test_assert_true(suite, test, result.present);
	lone_test_assert_unsigned_long_equal(suite, test, result.value, 11);
}

static LONE_TEST_FUNCTION(test_lone_bits_find_first_one_unaligned_word)
{
	unsigned char buffer[24] __attribute__((aligned(sizeof(unsigned long)))) = {
		/* [0..2] padding before bitmap */
		0, 0, 0,
		/* [3..7] leading bytes: 5 unaligned bytes, all clear */
		0, 0, 0, 0, 0,
		/* [8..15] one aligned word */
		0, 0, 128, 0, 0, 0, 0, 0,
		/* [16..19] trailing bytes */
		0, 0, 0, 0,
		/* [20..23] unused */
		0, 0, 0, 0,
	};

	/* bitmap byte 7 = buffer[10] = 128 = 0b10000000, first one at bit 0 within byte */
	/* expected bit index: 7 * 8 + 0 = 56 */
	struct lone_optional_size result = lone_bits_find_first_one(buffer + 3, 17);
	lone_test_assert_true(suite, test, result.present);
	lone_test_assert_unsigned_long_equal(suite, test, result.value, 56);
}

static LONE_TEST_FUNCTION(test_lone_bits_find_first_zero_unaligned_leading)
{
	unsigned char buffer[24] __attribute__((aligned(sizeof(unsigned long)))) = {
		/* [0..2] padding before bitmap */
		0, 0, 0,
		/* [3..15] bitmap: 13 bytes passed to find function */
		255, 239, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		/* [16..23] unused */
		0, 0, 0, 0, 0, 0, 0, 0,
	};

	/* bitmap byte 1 = 239 = 0b11101111, first zero at bit 3 within byte */
	/* expected bit index: 1 * 8 + 3 = 11 */
	struct lone_optional_size result = lone_bits_find_first_zero(buffer + 3, 13);
	lone_test_assert_true(suite, test, result.present);
	lone_test_assert_unsigned_long_equal(suite, test, result.value, 11);
}

static LONE_TEST_FUNCTION(test_lone_bits_find_first_zero_unaligned_word)
{
	unsigned char buffer[24] __attribute__((aligned(sizeof(unsigned long)))) = {
		/* [0..2] padding before bitmap */
		0, 0, 0,
		/* [3..7] leading bytes: 5 unaligned bytes, all set */
		255, 255, 255, 255, 255,
		/* [8..15] one aligned word */
		255, 255, 127, 255, 255, 255, 255, 255,
		/* [16..19] trailing bytes */
		255, 255, 255, 255,
		/* [20..23] unused */
		0, 0, 0, 0,
	};

	/* bitmap byte 7 = buffer[10] = 127 = 0b01111111, first zero at bit 0 within byte */
	/* expected bit index: 7 * 8 + 0 = 56 */
	struct lone_optional_size result = lone_bits_find_first_zero(buffer + 3, 17);
	lone_test_assert_true(suite, test, result.present);
	lone_test_assert_unsigned_long_equal(suite, test, result.value, 56);
}

long lone(int argc, char **argv, char **envp, struct lone_auxiliary_vector *auxv)
{

	static struct lone_test_case cases[] = {

		LONE_TEST_CASE("lone/bits/get",                test_lone_bits_get),
		LONE_TEST_CASE("lone/bits/get/first-bit",      test_lone_bits_get_first_bit),
		LONE_TEST_CASE("lone/bits/get/last-bit",       test_lone_bits_get_last_bit),
		LONE_TEST_CASE("lone/bits/get/byte-boundary",  test_lone_bits_get_byte_boundary),

		LONE_TEST_CASE("lone/bits/set",           test_lone_bits_set),
		LONE_TEST_CASE("lone/bits/set/and-clear", test_lone_bits_set_and_clear),

		LONE_TEST_CASE("lone/bits/mark",                  test_lone_bits_mark),
		LONE_TEST_CASE("lone/bits/clear",                 test_lone_bits_clear),
		LONE_TEST_CASE("lone/bits/mark-clear-round-trip", test_lone_bits_mark_clear_round_trip),

		LONE_TEST_CASE("lone/bits/find-first/one",               test_lone_bits_find_first_one),
		LONE_TEST_CASE("lone/bits/find-first/one/leading",       test_lone_bits_find_first_one_leading),
		LONE_TEST_CASE("lone/bits/find-first/one/trailing",      test_lone_bits_find_first_one_trailing),
		LONE_TEST_CASE("lone/bits/find-first/one/absent",        test_lone_bits_find_first_one_absent),
		LONE_TEST_CASE("lone/bits/find-first/one/word-plus-trailing",
				test_lone_bits_find_first_one_word_plus_trailing),
		LONE_TEST_CASE("lone/bits/find-first/one/small",         test_lone_bits_find_first_one_small),
		LONE_TEST_CASE("lone/bits/find-first/one/second-word",   test_lone_bits_find_first_one_second_word),

		LONE_TEST_CASE("lone/bits/find-first/zero",               test_lone_bits_find_first_zero),
		LONE_TEST_CASE("lone/bits/find-first/zero/leading",       test_lone_bits_find_first_zero_leading),
		LONE_TEST_CASE("lone/bits/find-first/zero/trailing",      test_lone_bits_find_first_zero_trailing),
		LONE_TEST_CASE("lone/bits/find-first/zero/absent",        test_lone_bits_find_first_zero_absent),
		LONE_TEST_CASE("lone/bits/find-first/zero/word-plus-trailing",
				test_lone_bits_find_first_zero_word_plus_trailing),
		LONE_TEST_CASE("lone/bits/find-first/zero/small",         test_lone_bits_find_first_zero_small),
		LONE_TEST_CASE("lone/bits/find-first/zero/second-word",   test_lone_bits_find_first_zero_second_word),

		LONE_TEST_CASE("lone/bits/find-first/one/unaligned/leading",
				test_lone_bits_find_first_one_unaligned_leading),
		LONE_TEST_CASE("lone/bits/find-first/one/unaligned/word",
				test_lone_bits_find_first_one_unaligned_word),
		LONE_TEST_CASE("lone/bits/find-first/zero/unaligned/leading",
				test_lone_bits_find_first_zero_unaligned_leading),
		LONE_TEST_CASE("lone/bits/find-first/zero/unaligned/word",
				test_lone_bits_find_first_zero_unaligned_word),

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
