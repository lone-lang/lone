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

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Adversarial input tests verify the reader rejects malformed         │
   │    input cleanly: reader.status.error is set after the read and        │
   │    the reader returns nil without exiting the process.                 │
   │                                                                        │
   │    Most tests fit in a fixed buffer via lone_lisp_reader_for_bytes,    │
   │    keeping the inputs visible at the call site. The                    │
   │    text-unterminated-large case uses a pipe so the buffer must grow    │
   │    before the lexer hits the missing closing quote, exercising the     │
   │    error path under fill_buffer.                                       │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
#define READER_ERROR_BYTES_TEST(name, literal)                                                    \
static LONE_TEST_FUNCTION(test_reader_error_##name)                                               \
{                                                                                                 \
	struct reader_test_context *ctx = test->context;                                          \
	struct lone_lisp_reader reader;                                                           \
	unsigned char input[] = literal;                                                          \
	struct lone_bytes bytes = { .pointer = input, .count = sizeof(input) - 1 };               \
                                                                                                  \
	lone_lisp_reader_for_bytes(ctx->lone, &reader, bytes);                                    \
	(void) lone_lisp_read(ctx->lone, &reader);                                                \
                                                                                                  \
	lone_test_assert_true(suite, test, reader.status.error);                                  \
}

READER_ERROR_BYTES_TEST(text_unterminated,          "\"abc")
READER_ERROR_BYTES_TEST(text_backslash_at_eof,      "\"abc\\")
READER_ERROR_BYTES_TEST(text_x_no_hex,              "\"\\x\"")
READER_ERROR_BYTES_TEST(text_x_non_hex,             "\"\\xZZ\"")
READER_ERROR_BYTES_TEST(text_invalid_escape,        "\"\\z\"")
READER_ERROR_BYTES_TEST(text_no_separator,          "\"abc\"x")
READER_ERROR_BYTES_TEST(bytes_literal_unterminated, "b\"abc")
READER_ERROR_BYTES_TEST(bytes_literal_no_separator, "b\"abc\"x")
READER_ERROR_BYTES_TEST(number_no_separator,        "123abc")
READER_ERROR_BYTES_TEST(symbol_quote_terminator,    "abc\"")
READER_ERROR_BYTES_TEST(symbol_comment_terminator,  "abc;")
READER_ERROR_BYTES_TEST(list_unterminated,          "(a b")
READER_ERROR_BYTES_TEST(list_mismatched_square,     "(a]")
READER_ERROR_BYTES_TEST(list_mismatched_curly,      "(a}")
READER_ERROR_BYTES_TEST(vector_unterminated,        "[a b")
READER_ERROR_BYTES_TEST(vector_mismatched_round,    "[a)")
READER_ERROR_BYTES_TEST(vector_mismatched_curly,    "[a}")
READER_ERROR_BYTES_TEST(table_unterminated,         "{a 1")
READER_ERROR_BYTES_TEST(table_mismatched_round,     "{a 1)")
READER_ERROR_BYTES_TEST(table_mismatched_square,    "{a 1]")
READER_ERROR_BYTES_TEST(table_odd_count,            "{a 1 b}")
READER_ERROR_BYTES_TEST(closing_round,              ")")
READER_ERROR_BYTES_TEST(closing_square,             "]")
READER_ERROR_BYTES_TEST(closing_curly,              "}")

static LONE_TEST_FUNCTION(test_reader_error_text_unterminated_large)
{
	struct reader_test_context *ctx = test->context;
	struct lone_lisp *lone = ctx->lone;
	struct lone_lisp_reader reader;
	size_t i;
	int fds[2];
	unsigned char prefix[] = { '"' };
	unsigned char pipe_data[5000];

	for (i = 0; i < sizeof(pipe_data); ++i) { pipe_data[i] = 'x'; }

	if (!lone_test_assert_long_equal(suite, test,
			pipe_reader_open(lone, &reader, fds), 0)) { return; }

	seed_buffer(&reader, prefix, sizeof(prefix));
	pipe_write_then_close(fds[1], pipe_data, sizeof(pipe_data));

	(void) lone_lisp_read(lone, &reader);

	lone_test_assert_true(suite, test, reader.status.error);

	linux_close(fds[0]);
	lone_lisp_reader_finalize(lone, &reader);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    EOF boundary tests verify graceful handling of input that has no    │
   │    token but is not malformed. status.error stays false and            │
   │    status.end_of_input is set so callers can distinguish clean         │
   │    end-of-input from parse errors.                                     │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
#define READER_EOF_BYTES_TEST(name, literal)                                                      \
static LONE_TEST_FUNCTION(test_reader_eof_##name)                                                 \
{                                                                                                 \
	struct reader_test_context *ctx = test->context;                                          \
	struct lone_lisp_reader reader;                                                           \
	unsigned char input[] = literal;                                                          \
	struct lone_bytes bytes = { .pointer = input, .count = sizeof(input) - 1 };               \
                                                                                                  \
	lone_lisp_reader_for_bytes(ctx->lone, &reader, bytes);                                    \
	(void) lone_lisp_read(ctx->lone, &reader);                                                \
                                                                                                  \
	lone_test_assert_true(suite, test, !reader.status.error);                                 \
	lone_test_assert_true(suite, test, reader.status.end_of_input);                           \
}

READER_EOF_BYTES_TEST(empty,              "")
READER_EOF_BYTES_TEST(whitespace_only,    "  \t\n  ")
READER_EOF_BYTES_TEST(comment,            ";line comment\n")
READER_EOF_BYTES_TEST(comment_no_newline, ";trailing")

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Scale tests stress sustained correctness under volume: large        │
   │    single tokens that force several buffer doublings, many small       │
   │    tokens read in a loop, and skip-paths fed pathological volumes      │
   │    of comments and whitespace. The pipe-based variants additionally    │
   │    exercise repeated fill_buffer invocations.                          │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static LONE_TEST_FUNCTION(test_reader_scale_huge_symbol)
{
	struct reader_test_context *ctx = test->context;
	struct lone_lisp *lone = ctx->lone;
	struct lone_lisp_reader reader;
	struct lone_lisp_value value;
	struct lone_bytes name;
	size_t i;
	int fds[2];
	unsigned char pipe_data[65536];

	for (i = 0; i < sizeof(pipe_data); ++i) { pipe_data[i] = 'x'; }

	if (!lone_test_assert_long_equal(suite, test,
			pipe_reader_open(lone, &reader, fds), 0)) { return; }

	pipe_write_then_close(fds[1], pipe_data, sizeof(pipe_data));

	value = lone_lisp_read(lone, &reader);

	lone_test_assert_true(suite, test, !reader.status.error);
	lone_test_assert_true(suite, test, lone_lisp_is_symbol(lone, value));
	name = lone_lisp_bytes_of(lone, &value);
	lone_test_assert_unsigned_long_equal(suite, test, name.count, sizeof(pipe_data));

	linux_close(fds[0]);
	lone_lisp_reader_finalize(lone, &reader);
}

static LONE_TEST_FUNCTION(test_reader_scale_huge_text)
{
	struct reader_test_context *ctx = test->context;
	struct lone_lisp *lone = ctx->lone;
	struct lone_lisp_reader reader;
	struct lone_lisp_value value;
	struct lone_bytes content;
	size_t i;
	int fds[2];
	unsigned char prefix[] = { '"' };
	unsigned char pipe_data[65536];

	for (i = 0; i < sizeof(pipe_data) - 1; ++i) { pipe_data[i] = 'x'; }
	pipe_data[sizeof(pipe_data) - 1] = '"';

	if (!lone_test_assert_long_equal(suite, test,
			pipe_reader_open(lone, &reader, fds), 0)) { return; }

	seed_buffer(&reader, prefix, sizeof(prefix));
	pipe_write_then_close(fds[1], pipe_data, sizeof(pipe_data));

	value = lone_lisp_read(lone, &reader);

	lone_test_assert_true(suite, test, !reader.status.error);
	lone_test_assert_true(suite, test, lone_lisp_is_text(lone, value));
	content = lone_lisp_bytes_of(lone, &value);
	lone_test_assert_unsigned_long_equal(suite, test,
		content.count, sizeof(pipe_data) - 1);

	linux_close(fds[0]);
	lone_lisp_reader_finalize(lone, &reader);
}

static LONE_TEST_FUNCTION(test_reader_scale_huge_bytes_literal)
{
	struct reader_test_context *ctx = test->context;
	struct lone_lisp *lone = ctx->lone;
	struct lone_lisp_reader reader;
	struct lone_lisp_value value;
	struct lone_bytes content;
	size_t i;
	int fds[2];
	unsigned char prefix[] = { 'b', '"' };
	unsigned char pipe_data[65536];

	for (i = 0; i < sizeof(pipe_data) - 1; ++i) { pipe_data[i] = 'x'; }
	pipe_data[sizeof(pipe_data) - 1] = '"';

	if (!lone_test_assert_long_equal(suite, test,
			pipe_reader_open(lone, &reader, fds), 0)) { return; }

	seed_buffer(&reader, prefix, sizeof(prefix));
	pipe_write_then_close(fds[1], pipe_data, sizeof(pipe_data));

	value = lone_lisp_read(lone, &reader);

	lone_test_assert_true(suite, test, !reader.status.error);
	lone_test_assert_true(suite, test, lone_lisp_is_bytes(lone, value));
	content = lone_lisp_bytes_of(lone, &value);
	lone_test_assert_unsigned_long_equal(suite, test,
		content.count, sizeof(pipe_data) - 1);

	linux_close(fds[0]);
	lone_lisp_reader_finalize(lone, &reader);
}

static LONE_TEST_FUNCTION(test_reader_scale_many_tokens)
{
	struct reader_test_context *ctx = test->context;
	struct lone_lisp *lone = ctx->lone;
	struct lone_lisp_reader reader;
	struct lone_lisp_value value;
	struct lone_bytes bytes;
	size_t i, count;
	unsigned char input[20000];

	/* "a a a a ... " with 10000 occurrences of 'a' separated by spaces. */
	for (i = 0; i < 10000; ++i) {
		input[2 * i]     = 'a';
		input[2 * i + 1] = ' ';
	}
	bytes.pointer = input;
	bytes.count   = sizeof(input);

	lone_lisp_reader_for_bytes(lone, &reader, bytes);

	count = 0;
	while (1) {
		value = lone_lisp_read(lone, &reader);
		if (reader.status.error) { break; }
		if (reader.status.end_of_input) { break; }
		if (lone_lisp_is_symbol(lone, value)) { ++count; }
	}

	lone_test_assert_true(suite, test, !reader.status.error);
	lone_test_assert_unsigned_long_equal(suite, test, count, 10000);
}

static LONE_TEST_FUNCTION(test_reader_scale_long_flat_list)
{
	struct reader_test_context *ctx = test->context;
	struct lone_lisp *lone = ctx->lone;
	struct lone_lisp_reader reader;
	struct lone_lisp_value value, list;
	struct lone_bytes bytes;
	size_t i, count;
	unsigned char input[10001];

	/* "(1 1 1 ... 1)" with 5000 ones. */
	input[0] = '(';
	for (i = 0; i < 5000; ++i) {
		input[1 + 2 * i] = '1';
		if (i < 4999) { input[2 + 2 * i] = ' '; }
	}
	input[10000] = ')';

	bytes.pointer = input;
	bytes.count   = sizeof(input);

	lone_lisp_reader_for_bytes(lone, &reader, bytes);

	value = lone_lisp_read(lone, &reader);

	lone_test_assert_true(suite, test, !reader.status.error);
	lone_test_assert_true(suite, test, lone_lisp_is_list(lone, value));

	list  = value;
	count = 0;
	while (!lone_lisp_is_nil(list)) {
		++count;
		list = lone_lisp_list_rest(lone, list);
	}
	lone_test_assert_unsigned_long_equal(suite, test, count, 5000);
}

static LONE_TEST_FUNCTION(test_reader_scale_comment_storm)
{
	struct reader_test_context *ctx = test->context;
	struct lone_lisp *lone = ctx->lone;
	struct lone_lisp_reader reader;
	struct lone_lisp_value value;
	struct lone_bytes bytes;
	size_t i;
	unsigned char input[3001];

	/* 1000 lines of ";c\n" then a final 'x' token. */
	for (i = 0; i < 1000; ++i) {
		input[3 * i]     = ';';
		input[3 * i + 1] = 'c';
		input[3 * i + 2] = '\n';
	}
	input[3000] = 'x';

	bytes.pointer = input;
	bytes.count   = sizeof(input);

	lone_lisp_reader_for_bytes(lone, &reader, bytes);

	value = lone_lisp_read(lone, &reader);

	lone_test_assert_true(suite, test, !reader.status.error);
	lone_test_assert_true(suite, test, lone_lisp_is_symbol(lone, value));
}

static LONE_TEST_FUNCTION(test_reader_scale_whitespace_storm)
{
	struct reader_test_context *ctx = test->context;
	struct lone_lisp *lone = ctx->lone;
	struct lone_lisp_reader reader;
	struct lone_lisp_value value;
	struct lone_bytes bytes;
	size_t i;
	unsigned char input[20000];

	/* Cycle through space, tab, newline. Final byte is the token. */
	for (i = 0; i < sizeof(input) - 1; ++i) {
		switch (i % 3) {
		case 0:  input[i] = ' ';  break;
		case 1:  input[i] = '\t'; break;
		case 2:  input[i] = '\n'; break;
		}
	}
	input[sizeof(input) - 1] = 'x';

	bytes.pointer = input;
	bytes.count   = sizeof(input);

	lone_lisp_reader_for_bytes(lone, &reader, bytes);

	value = lone_lisp_read(lone, &reader);

	lone_test_assert_true(suite, test, !reader.status.error);
	lone_test_assert_true(suite, test, lone_lisp_is_symbol(lone, value));
}

static LONE_TEST_FUNCTION(test_reader_scale_sustained_reads)
{
	struct reader_test_context *ctx = test->context;
	struct lone_lisp *lone = ctx->lone;
	struct lone_lisp_reader reader;
	struct lone_lisp_value value;
	size_t i, count;
	int fds[2];
	unsigned char pipe_data[20000];

	/* "a a a ... " — 10000 tokens piped through. The reader must
	 * call fill_buffer many times across the stream. */
	for (i = 0; i < 10000; ++i) {
		pipe_data[2 * i]     = 'a';
		pipe_data[2 * i + 1] = ' ';
	}

	if (!lone_test_assert_long_equal(suite, test,
			pipe_reader_open(lone, &reader, fds), 0)) { return; }

	pipe_write_then_close(fds[1], pipe_data, sizeof(pipe_data));

	count = 0;
	while (1) {
		value = lone_lisp_read(lone, &reader);
		if (reader.status.error) { break; }
		if (reader.status.end_of_input) { break; }
		if (lone_lisp_is_symbol(lone, value)) { ++count; }
	}

	lone_test_assert_true(suite, test, !reader.status.error);
	lone_test_assert_unsigned_long_equal(suite, test, count, 10000);

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

		LONE_TEST_CASE("reader/error/text/unterminated",
				test_reader_error_text_unterminated),
		LONE_TEST_CASE("reader/error/text/backslash-at-eof",
				test_reader_error_text_backslash_at_eof),
		LONE_TEST_CASE("reader/error/text/x-no-hex",
				test_reader_error_text_x_no_hex),
		LONE_TEST_CASE("reader/error/text/x-non-hex",
				test_reader_error_text_x_non_hex),
		LONE_TEST_CASE("reader/error/text/invalid-escape",
				test_reader_error_text_invalid_escape),
		LONE_TEST_CASE("reader/error/text/no-separator",
				test_reader_error_text_no_separator),
		LONE_TEST_CASE("reader/error/text/unterminated-large",
				test_reader_error_text_unterminated_large),
		LONE_TEST_CASE("reader/error/bytes-literal/unterminated",
				test_reader_error_bytes_literal_unterminated),
		LONE_TEST_CASE("reader/error/bytes-literal/no-separator",
				test_reader_error_bytes_literal_no_separator),
		LONE_TEST_CASE("reader/error/number/no-separator",
				test_reader_error_number_no_separator),
		LONE_TEST_CASE("reader/error/symbol/quote-terminator",
				test_reader_error_symbol_quote_terminator),
		LONE_TEST_CASE("reader/error/symbol/comment-terminator",
				test_reader_error_symbol_comment_terminator),
		LONE_TEST_CASE("reader/error/list/unterminated",
				test_reader_error_list_unterminated),
		LONE_TEST_CASE("reader/error/list/mismatched/square",
				test_reader_error_list_mismatched_square),
		LONE_TEST_CASE("reader/error/list/mismatched/curly",
				test_reader_error_list_mismatched_curly),
		LONE_TEST_CASE("reader/error/vector/unterminated",
				test_reader_error_vector_unterminated),
		LONE_TEST_CASE("reader/error/vector/mismatched/round",
				test_reader_error_vector_mismatched_round),
		LONE_TEST_CASE("reader/error/vector/mismatched/curly",
				test_reader_error_vector_mismatched_curly),
		LONE_TEST_CASE("reader/error/table/unterminated",
				test_reader_error_table_unterminated),
		LONE_TEST_CASE("reader/error/table/mismatched/round",
				test_reader_error_table_mismatched_round),
		LONE_TEST_CASE("reader/error/table/mismatched/square",
				test_reader_error_table_mismatched_square),
		LONE_TEST_CASE("reader/error/table/odd-count",
				test_reader_error_table_odd_count),
		LONE_TEST_CASE("reader/error/closing-at-top-level/round",
				test_reader_error_closing_round),
		LONE_TEST_CASE("reader/error/closing-at-top-level/square",
				test_reader_error_closing_square),
		LONE_TEST_CASE("reader/error/closing-at-top-level/curly",
				test_reader_error_closing_curly),

		LONE_TEST_CASE("reader/eof/empty",
				test_reader_eof_empty),
		LONE_TEST_CASE("reader/eof/whitespace-only",
				test_reader_eof_whitespace_only),
		LONE_TEST_CASE("reader/eof/comment",
				test_reader_eof_comment),
		LONE_TEST_CASE("reader/eof/comment-no-newline",
				test_reader_eof_comment_no_newline),

		LONE_TEST_CASE("reader/scale/huge-symbol",
				test_reader_scale_huge_symbol),
		LONE_TEST_CASE("reader/scale/huge-text",
				test_reader_scale_huge_text),
		LONE_TEST_CASE("reader/scale/huge-bytes-literal",
				test_reader_scale_huge_bytes_literal),
		LONE_TEST_CASE("reader/scale/many-tokens",
				test_reader_scale_many_tokens),
		LONE_TEST_CASE("reader/scale/long-flat-list",
				test_reader_scale_long_flat_list),
		LONE_TEST_CASE("reader/scale/comment-storm",
				test_reader_scale_comment_storm),
		LONE_TEST_CASE("reader/scale/whitespace-storm",
				test_reader_scale_whitespace_storm),
		LONE_TEST_CASE("reader/scale/sustained-reads",
				test_reader_scale_sustained_reads),

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
