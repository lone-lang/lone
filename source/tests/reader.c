/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/definitions.h>
#include <lone/types.h>
#include <lone/linux.h>
#include <lone/test.h>
#include <lone/memory/functions.h>
#include <lone/system.h>
#include <lone/lisp.h>
#include <lone/lisp/reader.h>

struct reader_test_context {
	struct lone_lisp *lone;
};

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Reader buffer out-of-bounds on re-entry.                            │
   │                                                                        │
   │    The reader's fill_buffer used to pass the full buffer size as       │
   │    the read length, starting the read at buffer + position.write.      │
   │    Any call with position.write > 0 therefore asked the kernel to      │
   │    write past the end of the buffer.                                   │
   │                                                                        │
   │    This test mimics the aftermath of a short first fill by seeding     │
   │    the reader state directly, then stages a full second batch in       │
   │    the pipe before driving fill_buffer again. The correct algorithm    │
   │    fills the grown buffer contiguously. The buggy one leaves a gap     │
   │    of zeroes starting at the old buffer's end.                         │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static LONE_TEST_FUNCTION(test_regression_reader_fill_buffer_reentry)
{
	struct reader_test_context *ctx = test->context;
	struct lone_lisp *lone = ctx->lone;
	struct lone_lisp_reader reader;
	unsigned char data[4100];
	int fds[2];
	long ret;
	size_t i;

	for (i = 0; i < sizeof(data); ++i) { data[i] = 'x'; }

	ret = linux_pipe2(fds, 0);
	if (!lone_test_assert_long_equal(suite, test, ret, 0)) { return; }

	lone_lisp_reader_for_file_descriptor(lone, &reader, 4096, fds[0]);
	lone_memory_zero(reader.buffer.bytes.pointer, reader.buffer.bytes.count);

	/* Pretend a previous short fill_buffer left "42 " in the buffer.
	 * The trailing whitespace is a token separator so the reader will
	 * consume it and advance past the staged bytes on the first read. */
	reader.buffer.bytes.pointer[0] = '4';
	reader.buffer.bytes.pointer[1] = '2';
	reader.buffer.bytes.pointer[2] = ' ';
	reader.buffer.position.read  = 0;
	reader.buffer.position.write = 3;

	linux_write(fds[1], data, sizeof(data));
	linux_close(fds[1]);

	/* Consume the staged number; advances position.read to position.write. */
	(void) lone_lisp_read(lone, &reader);

	/* Trigger fill_buffer with position.write > 0. */
	(void) lone_lisp_read(lone, &reader);

	lone_test_assert_true(suite, test,
		reader.buffer.position.write <= reader.buffer.bytes.count);

	/* In the grown buffer every byte from the pipe must be contiguous
	 * after the staged prefix. The buggy path lost the bytes that the
	 * kernel wrote past the original 4096-byte buffer, so the byte at
	 * offset 4097 in the grown buffer was zero instead of 'x'. */
	lone_test_assert_unsigned_char_equal(suite, test, reader.buffer.bytes.pointer[4097], 'x');

	linux_close(fds[0]);
	lone_lisp_reader_finalize(lone, &reader);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Token use-after-free across a buffer growth.                        │
   │                                                                        │
   │    consume_symbol and consume_number cached a pointer to the token's   │
   │    first byte and held it across a loop of peeks. Any of those peeks   │
   │    could drive fill_buffer, which reallocates when the buffer fills.   │
   │    The reallocation moves the backing storage across the slab/mmap     │
   │    boundary and zeroes the old slab, so the cached pointer becomes     │
   │    a dangling reference to freed memory, which intern then reads       │
   │    instead of the token's actual characters.                           │
   │                                                                        │
   │    Stage a single leading byte so consume_symbol starts immediately,   │
   │    supply the rest through the pipe. The second peek in the consume    │
   │    loop drives the grow-and-move. The correct algorithm recalculates   │
   │    the pointer from the offset. The buggy one returns a symbol whose   │
   │    name is all zeroes.                                                 │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static LONE_TEST_FUNCTION(test_regression_reader_symbol_across_growth)
{
	struct reader_test_context *ctx = test->context;
	struct lone_lisp *lone = ctx->lone;
	struct lone_lisp_reader reader;
	struct lone_lisp_value value;
	struct lone_bytes name;
	unsigned char data[5000];
	int fds[2];
	long ret;
	size_t i;

	for (i = 0; i < sizeof(data); ++i) { data[i] = 'x'; }

	ret = linux_pipe2(fds, 0);
	if (!lone_test_assert_long_equal(suite, test, ret, 0)) { return; }

	lone_lisp_reader_for_file_descriptor(lone, &reader, 4096, fds[0]);
	lone_memory_zero(reader.buffer.bytes.pointer, reader.buffer.bytes.count);

	/* Stage a single symbol byte. consume_symbol's initial peek lands
	 * here and records start; the loop's next peek needs more input
	 * and drives fill_buffer, which grows the buffer past 4096 bytes
	 * and moves it across the slab/mmap boundary. */
	reader.buffer.bytes.pointer[0] = 'x';
	reader.buffer.position.read  = 0;
	reader.buffer.position.write = 1;

	linux_write(fds[1], data, sizeof(data));
	linux_close(fds[1]);

	value = lone_lisp_read(lone, &reader);

	lone_test_assert_true(suite, test, lone_lisp_is_symbol(lone, value));

	name = lone_lisp_bytes_of(lone, &value);
	lone_test_assert_unsigned_long_equal(suite, test, name.count, 1 + sizeof(data));
	lone_test_assert_unsigned_char_equal(suite, test, name.pointer[0], 'x');
	lone_test_assert_unsigned_char_equal(suite, test,
		name.pointer[name.count - 1], 'x');
	/* Spot-check a byte near the growth point. The buggy code read
	 * this byte from the zeroed old slab instead of from the token's
	 * real position in the grown buffer. */
	lone_test_assert_unsigned_char_equal(suite, test,
		name.pointer[4097], 'x');

	linux_close(fds[0]);
	lone_lisp_reader_finalize(lone, &reader);
}

long lone(int argc, char **argv, char **envp, struct lone_auxiliary_vector *auxv)
{
	void *stack = __builtin_frame_address(0);
	struct lone_system system;
	struct lone_lisp lone_interpreter;
	struct reader_test_context context;
	struct lone_test_suite suite;
	enum lone_test_result result;

	static struct lone_test_case cases[] = {
		LONE_TEST_CASE("regression/reader/fill-buffer/reentry",
				test_regression_reader_fill_buffer_reentry),
		LONE_TEST_CASE("regression/reader/symbol/across-growth",
				test_regression_reader_symbol_across_growth),
		LONE_TEST_CASE_NULL(),
	};

	lone_system_initialize(&system, auxv);
	lone_lisp_initialize(&lone_interpreter, &system, stack);

	context.lone = &lone_interpreter;
	cases[0].context = &context;
	cases[1].context = &context;

	suite = (struct lone_test_suite) LONE_TEST_SUITE(cases);

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

#include <lone/architecture/linux/entry.c>
