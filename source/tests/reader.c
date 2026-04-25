/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/definitions.h>
#include <lone/types.h>
#include <lone/linux.h>
#include <lone/test.h>
#include <lone/memory/functions.h>
#include <lone/system.h>
#include <lone/lisp.h>
#include <lone/lisp/reader.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Reader test helpers.                                                │
   │                                                                        │
   │    pipe_reader_open creates a fresh pipe and a reader bound to its     │
   │    read end with a 4096-byte buffer, zeroed for deterministic state.   │
   │                                                                        │
   │    seed_buffer overlays bytes at the start of the reader's buffer      │
   │    and sets the write head past them. The reader will see the seeded   │
   │    bytes as if they had been read from the pipe.                       │
   │                                                                        │
   │    pipe_write_then_close stages bytes in the pipe and closes the       │
   │    write end so subsequent reads see EOF after the staged bytes.       │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static long pipe_reader_open(struct lone_lisp *lone,
		struct lone_lisp_reader *reader, int fds[2])
{
	long ret;

	ret = linux_pipe2(fds, 0);
	if (ret == 0) {
		lone_lisp_reader_for_file_descriptor(lone, reader, 4096, fds[0]);
		lone_memory_zero(reader->buffer.bytes.pointer, reader->buffer.bytes.count);
	}

	return ret;
}

static void seed_buffer(struct lone_lisp_reader *reader,
		unsigned char *bytes, size_t count)
{
	lone_memory_move(bytes, reader->buffer.bytes.pointer, count);
	reader->buffer.position.read  = 0;
	reader->buffer.position.write = count;
}

static long pipe_write_then_close(int write_fd,
		unsigned char *bytes, size_t count)
{
	long ret;

	ret = linux_write(write_fd, bytes, count);
	linux_close(write_fd);
	return ret;
}

struct reader_test_context {
	struct lone_lisp *lone;
};

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Reader buffer OOB on re-entry.                                      │
   │                                                                        │
   │    The reader's fill_buffer used to pass the full buffer size as       │
   │    the read length, starting the read at buffer + position.write.      │
   │    Any call with position.write > 0 therefore asked the kernel to      │
   │    write past the end of the buffer.                                   │
   │                                                                        │
   │    This test mimics the aftermath of a short first fill by seeding     │
   │    the reader state directly, then stages a full second batch in       │
   │    the pipe before driving fill_buffer again. A correct                │
   │    implementation fills the grown buffer contiguously; the buggy       │
   │    one leaves a gap of zeroes starting at the old buffer's end.        │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static LONE_TEST_FUNCTION(test_regression_reader_fill_buffer_reentry)
{
	struct reader_test_context *ctx = test->context;
	struct lone_lisp *lone = ctx->lone;
	struct lone_lisp_reader reader;
	unsigned char pipe_data[4100];
	unsigned char prefix[] = { '4', '2', ' ' };
	int fds[2];
	size_t i;

	for (i = 0; i < sizeof(pipe_data); ++i) { pipe_data[i] = 'x'; }

	if (!lone_test_assert_long_equal(suite, test,
			pipe_reader_open(lone, &reader, fds), 0)) { return; }

	seed_buffer(&reader, prefix, sizeof(prefix));
	pipe_write_then_close(fds[1], pipe_data, sizeof(pipe_data));

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
	lone_test_assert_unsigned_char_equal(suite, test,
		reader.buffer.bytes.pointer[4097], 'x');

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
	unsigned char pipe_data[5000];
	unsigned char prefix[] = { 'x' };
	int fds[2];
	size_t i;

	for (i = 0; i < sizeof(pipe_data); ++i) { pipe_data[i] = 'x'; }

	if (!lone_test_assert_long_equal(suite, test,
			pipe_reader_open(lone, &reader, fds), 0)) { return; }

	/* Stage a single symbol byte. consume_symbol's initial peek lands
	 * here and records start; the loop's next peek needs more input
	 * and drives fill_buffer, which grows the buffer past 4096 bytes
	 * and moves it across the slab/mmap boundary. */
	seed_buffer(&reader, prefix, sizeof(prefix));
	pipe_write_then_close(fds[1], pipe_data, sizeof(pipe_data));

	value = lone_lisp_read(lone, &reader);

	lone_test_assert_true(suite, test, lone_lisp_is_symbol(lone, value));

	name = lone_lisp_bytes_of(lone, &value);
	lone_test_assert_unsigned_long_equal(suite, test,
		name.count, sizeof(prefix) + sizeof(pipe_data));
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

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    consume_number across a buffer growth.                              │
   │                                                                        │
   │    The pointer-recalc fix in ed0edc0e applied to consume_symbol and    │
   │    consume_number, but only the symbol path had a regression test.     │
   │    Stage a sign byte so consume_number captures the start, then pipe   │
   │    a long run of digits ending in a non-zero so a correct parse        │
   │    yields a small known value while a stale-pointer parse reads        │
   │    zeroed slab memory and either wraps catastrophically or aborts      │
   │    on overflow inside lone_lisp_integer_parse.                         │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static LONE_TEST_FUNCTION(test_reader_growth_number)
{
	struct reader_test_context *ctx = test->context;
	struct lone_lisp *lone = ctx->lone;
	struct lone_lisp_reader reader;
	struct lone_lisp_value value;
	size_t i;
	int fds[2];
	unsigned char prefix[] = { '+' };
	unsigned char pipe_data[5000];

	/* All zeros keep integer_parse at zero across the buffer growth.
	 * The final byte is a non-zero digit so a correct parse returns
	 * exactly that digit. */
	for (i = 0; i < sizeof(pipe_data) - 1; ++i) { pipe_data[i] = '0'; }
	pipe_data[sizeof(pipe_data) - 1] = '7';

	if (!lone_test_assert_long_equal(suite, test,
			pipe_reader_open(lone, &reader, fds), 0)) { return; }

	seed_buffer(&reader, prefix, sizeof(prefix));
	pipe_write_then_close(fds[1], pipe_data, sizeof(pipe_data));

	value = lone_lisp_read(lone, &reader);

	lone_test_assert_true(suite, test, lone_lisp_is_integer(lone, value));
	lone_test_assert_long_equal(suite, test, lone_lisp_integer_of(value), 7);

	linux_close(fds[0]);
	lone_lisp_reader_finalize(lone, &reader);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    consume_text across a buffer growth.                                │
   │                                                                        │
   │    consume_escaped_content scans the input twice: once to count        │
   │    bytes and escapes, then once to consume into a separately           │
   │    allocated output. Both passes can drive fill_buffer; both must      │
   │    survive a buffer relocation between or during them. Stage the       │
   │    opening quote, pipe a long body and a closing quote, verify the     │
   │    resulting text matches byte for byte.                               │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static LONE_TEST_FUNCTION(test_reader_growth_text)
{
	struct reader_test_context *ctx = test->context;
	struct lone_lisp *lone = ctx->lone;
	struct lone_lisp_reader reader;
	struct lone_lisp_value value;
	struct lone_bytes content;
	size_t i;
	int fds[2];
	unsigned char prefix[] = { '"' };
	unsigned char pipe_data[5001];

	for (i = 0; i < sizeof(pipe_data) - 1; ++i) { pipe_data[i] = 'x'; }
	pipe_data[sizeof(pipe_data) - 1] = '"';

	if (!lone_test_assert_long_equal(suite, test,
			pipe_reader_open(lone, &reader, fds), 0)) { return; }

	seed_buffer(&reader, prefix, sizeof(prefix));
	pipe_write_then_close(fds[1], pipe_data, sizeof(pipe_data));

	value = lone_lisp_read(lone, &reader);

	lone_test_assert_true(suite, test, lone_lisp_is_text(lone, value));
	content = lone_lisp_bytes_of(lone, &value);
	lone_test_assert_unsigned_long_equal(suite, test,
		content.count, sizeof(pipe_data) - 1);
	lone_test_assert_unsigned_char_equal(suite, test, content.pointer[0], 'x');
	lone_test_assert_unsigned_char_equal(suite, test,
		content.pointer[content.count - 1], 'x');
	/* Spot-check across the growth point. */
	lone_test_assert_unsigned_char_equal(suite, test,
		content.pointer[4096], 'x');

	linux_close(fds[0]);
	lone_lisp_reader_finalize(lone, &reader);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    consume_byte_literal across a buffer growth.                        │
   │                                                                        │
   │    Same input shape as the text test, but with the b" prefix so the    │
   │    lexer dispatches to consume_byte_literal. The escaped content       │
   │    helper is shared but the resulting value is frozen bytes, not       │
   │    text, so the path through bytes_transfer is exercised at scale.    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static LONE_TEST_FUNCTION(test_reader_growth_bytes_literal)
{
	struct reader_test_context *ctx = test->context;
	struct lone_lisp *lone = ctx->lone;
	struct lone_lisp_reader reader;
	struct lone_lisp_value value;
	struct lone_bytes content;
	size_t i;
	int fds[2];
	unsigned char prefix[] = { 'b', '"' };
	unsigned char pipe_data[5001];

	for (i = 0; i < sizeof(pipe_data) - 1; ++i) { pipe_data[i] = 'x'; }
	pipe_data[sizeof(pipe_data) - 1] = '"';

	if (!lone_test_assert_long_equal(suite, test,
			pipe_reader_open(lone, &reader, fds), 0)) { return; }

	seed_buffer(&reader, prefix, sizeof(prefix));
	pipe_write_then_close(fds[1], pipe_data, sizeof(pipe_data));

	value = lone_lisp_read(lone, &reader);

	lone_test_assert_true(suite, test, lone_lisp_is_bytes(lone, value));
	content = lone_lisp_bytes_of(lone, &value);
	lone_test_assert_unsigned_long_equal(suite, test,
		content.count, sizeof(pipe_data) - 1);
	lone_test_assert_unsigned_char_equal(suite, test, content.pointer[0], 'x');
	lone_test_assert_unsigned_char_equal(suite, test,
		content.pointer[content.count - 1], 'x');
	lone_test_assert_unsigned_char_equal(suite, test,
		content.pointer[4096], 'x');

	linux_close(fds[0]);
	lone_lisp_reader_finalize(lone, &reader);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Escape sequence straddling the buffer growth point.                 │
   │                                                                        │
   │    Place the backslash at the last position the original buffer can    │
   │    hold, force fill_buffer to grow when peeking the escape character.  │
   │    consume_escaped_content's inner peek_k for the escape's second      │
   │    byte runs through the relocated buffer. A correct decode places     │
   │    a newline at the post-prefix position; a bug that fails to follow   │
   │    the relocation would either decode the wrong byte or read freed    │
   │    memory.                                                             │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static LONE_TEST_FUNCTION(test_reader_growth_escape)
{
	struct reader_test_context *ctx = test->context;
	struct lone_lisp *lone = ctx->lone;
	struct lone_lisp_reader reader;
	struct lone_lisp_value value;
	struct lone_bytes content;
	size_t i;
	int fds[2];
	unsigned char prefix[4095];
	unsigned char pipe_data[102];

	prefix[0] = '"';
	for (i = 1; i < sizeof(prefix) - 1; ++i) { prefix[i] = 'x'; }
	prefix[sizeof(prefix) - 1] = '\\';

	pipe_data[0] = 'n';
	for (i = 1; i < sizeof(pipe_data) - 1; ++i) { pipe_data[i] = 'x'; }
	pipe_data[sizeof(pipe_data) - 1] = '"';

	if (!lone_test_assert_long_equal(suite, test,
			pipe_reader_open(lone, &reader, fds), 0)) { return; }

	seed_buffer(&reader, prefix, sizeof(prefix));
	pipe_write_then_close(fds[1], pipe_data, sizeof(pipe_data));

	value = lone_lisp_read(lone, &reader);

	lone_test_assert_true(suite, test, lone_lisp_is_text(lone, value));
	content = lone_lisp_bytes_of(lone, &value);
	/* Input body: 4093 'x' + '\\' + 'n' + 100 'x' = 4195 bytes
	 * One escape collapses '\\n' to a single newline, so output is 4194. */
	lone_test_assert_unsigned_long_equal(suite, test, content.count, 4194);
	lone_test_assert_unsigned_char_equal(suite, test, content.pointer[0], 'x');
	lone_test_assert_unsigned_char_equal(suite, test,
		content.pointer[4092], 'x');
	lone_test_assert_unsigned_char_equal(suite, test,
		content.pointer[4093], '\n');
	lone_test_assert_unsigned_char_equal(suite, test,
		content.pointer[4094], 'x');
	lone_test_assert_unsigned_char_equal(suite, test,
		content.pointer[content.count - 1], 'x');

	linux_close(fds[0]);
	lone_lisp_reader_finalize(lone, &reader);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Two consecutive tokens around a buffer growth.                      │
   │                                                                        │
   │    Read a small number from the pre-staged buffer first, then read a   │
   │    long symbol whose body is supplied by the pipe and forces growth.   │
   │    Verifies the reader keeps consistent state across token boundaries  │
   │    when growth happens between reads.                                  │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static LONE_TEST_FUNCTION(test_reader_growth_two_tokens)
{
	struct reader_test_context *ctx = test->context;
	struct lone_lisp *lone = ctx->lone;
	struct lone_lisp_reader reader;
	struct lone_lisp_value first, second;
	struct lone_bytes name;
	size_t i;
	int fds[2];
	unsigned char prefix[] = { '4', '2', ' ' };
	unsigned char pipe_data[5000];

	for (i = 0; i < sizeof(pipe_data); ++i) { pipe_data[i] = 'x'; }

	if (!lone_test_assert_long_equal(suite, test,
			pipe_reader_open(lone, &reader, fds), 0)) { return; }

	seed_buffer(&reader, prefix, sizeof(prefix));
	pipe_write_then_close(fds[1], pipe_data, sizeof(pipe_data));

	first = lone_lisp_read(lone, &reader);
	second = lone_lisp_read(lone, &reader);

	lone_test_assert_true(suite, test, lone_lisp_is_integer(lone, first));
	lone_test_assert_long_equal(suite, test, lone_lisp_integer_of(first), 42);

	lone_test_assert_true(suite, test, lone_lisp_is_symbol(lone, second));
	name = lone_lisp_bytes_of(lone, &second);
	lone_test_assert_unsigned_long_equal(suite, test,
		name.count, sizeof(pipe_data));
	lone_test_assert_unsigned_char_equal(suite, test,
		name.pointer[name.count - 1], 'x');

	linux_close(fds[0]);
	lone_lisp_reader_finalize(lone, &reader);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Symbols sized exactly at the growth boundary.                       │
   │                                                                        │
   │    The initial buffer is 4096 bytes. A 4095-byte token finishes        │
   │    without growth; 4096 fills the buffer exactly and triggers a        │
   │    grow that reads zero bytes; 4097 needs the grow plus a single       │
   │    byte more. Each case must produce a symbol of the input length.     │
   │    Off-by-one bugs in the growth threshold show up here.               │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
#define READER_GROWTH_BOUNDARY_TEST(N)                                                            \
static LONE_TEST_FUNCTION(test_reader_growth_exact_boundary_##N)                                  \
{                                                                                                 \
	struct reader_test_context *ctx = test->context;                                          \
	struct lone_lisp *lone = ctx->lone;                                                       \
	struct lone_lisp_reader reader;                                                           \
	struct lone_lisp_value value;                                                             \
	struct lone_bytes name;                                                                   \
	unsigned char pipe_data[N];                                                               \
	int fds[2];                                                                               \
	size_t i;                                                                                 \
                                                                                                  \
	for (i = 0; i < sizeof(pipe_data); ++i) { pipe_data[i] = 'x'; }                           \
                                                                                                  \
	if (!lone_test_assert_long_equal(suite, test,                                             \
			pipe_reader_open(lone, &reader, fds), 0)) { return; }                     \
                                                                                                  \
	pipe_write_then_close(fds[1], pipe_data, sizeof(pipe_data));                              \
                                                                                                  \
	value = lone_lisp_read(lone, &reader);                                                    \
                                                                                                  \
	lone_test_assert_true(suite, test, lone_lisp_is_symbol(lone, value));                     \
	name = lone_lisp_bytes_of(lone, &value);                                                  \
	lone_test_assert_unsigned_long_equal(suite, test, name.count, sizeof(pipe_data));         \
	lone_test_assert_unsigned_char_equal(suite, test, name.pointer[0], 'x');                  \
	lone_test_assert_unsigned_char_equal(suite, test,                                         \
		name.pointer[name.count - 1], 'x');                                               \
                                                                                                  \
	linux_close(fds[0]);                                                                      \
	lone_lisp_reader_finalize(lone, &reader);                                                 \
}

READER_GROWTH_BOUNDARY_TEST(4095)
READER_GROWTH_BOUNDARY_TEST(4096)
READER_GROWTH_BOUNDARY_TEST(4097)

long lone(int argc, char **argv, char **envp, struct lone_auxiliary_vector *auxv)
{
	void *stack = __builtin_frame_address(0);
	struct lone_system system;
	struct lone_lisp lone_interpreter;
	struct reader_test_context context;
	struct lone_test_suite suite;
	enum lone_test_result result;
	size_t i;

	static struct lone_test_case cases[] = {
		LONE_TEST_CASE("regression/reader/fill-buffer/reentry",
				test_regression_reader_fill_buffer_reentry),
		LONE_TEST_CASE("regression/reader/symbol/across-growth",
				test_regression_reader_symbol_across_growth),
		LONE_TEST_CASE("reader/growth/number",
				test_reader_growth_number),
		LONE_TEST_CASE("reader/growth/text",
				test_reader_growth_text),
		LONE_TEST_CASE("reader/growth/bytes-literal",
				test_reader_growth_bytes_literal),
		LONE_TEST_CASE("reader/growth/escape",
				test_reader_growth_escape),
		LONE_TEST_CASE("reader/growth/two-tokens",
				test_reader_growth_two_tokens),
		LONE_TEST_CASE("reader/growth/exact-boundary/4095",
				test_reader_growth_exact_boundary_4095),
		LONE_TEST_CASE("reader/growth/exact-boundary/4096",
				test_reader_growth_exact_boundary_4096),
		LONE_TEST_CASE("reader/growth/exact-boundary/4097",
				test_reader_growth_exact_boundary_4097),
		LONE_TEST_CASE_NULL(),
	};

	lone_system_initialize(&system, auxv);
	lone_lisp_initialize(&lone_interpreter, &system, stack);

	context.lone = &lone_interpreter;
	for (i = 0; cases[i].test; ++i) { cases[i].context = &context; }

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
