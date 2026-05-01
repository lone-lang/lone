/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/types.h>
#include <lone/lisp/heap.h>
#include <lone/lisp/utilities.h>

#include <lone/unicode.h>

#include <lone/memory/allocator.h>
#include <lone/memory/functions.h>

#include <lone/linux.h>

static void lone_lisp_text_validate_utf8(struct lone_bytes bytes,
		struct lone_lisp_heap_value *actual)
{
	struct lone_unicode_utf8_validation_result validation;

	validation = lone_unicode_utf8_validate(bytes);

	if (!validation.valid) { linux_exit(-1); }

	actual->as.text.code_point_count = validation.code_point_count;
	actual->code_point_count_cached = true;
}

struct lone_lisp_value lone_lisp_text_transfer(struct lone_lisp *lone,
		unsigned char *text, size_t length, bool should_deallocate)
{
	struct lone_lisp_heap_value *actual;
	struct lone_lisp_value value;

	if (length <= LONE_LISP_INLINE_MAX_LENGTH) {
		value = lone_lisp_inline_text_create(text, length);
		if (should_deallocate) {
			lone_memory_deallocate(lone->system, text, length + 1, 1, 1);
		}
		return value;
	}

	actual = lone_lisp_heap_allocate_value(lone);

	return lone_lisp_buffer_transfer(lone, actual, &actual->as.text.bytes,
			text, length, should_deallocate, LONE_LISP_TAG_TEXT);
}

struct lone_lisp_value lone_lisp_text_transfer_bytes(struct lone_lisp *lone,
		struct lone_bytes bytes, bool should_deallocate)
{
	return lone_lisp_text_transfer(lone, bytes.pointer, bytes.count, should_deallocate);
}

struct lone_lisp_value lone_lisp_text_copy(struct lone_lisp *lone, unsigned char *text, size_t length)
{
	struct lone_lisp_heap_value *actual;

	if (length <= LONE_LISP_INLINE_MAX_LENGTH) {
		return lone_lisp_inline_text_create(text, length);
	}

	actual = lone_lisp_heap_allocate_value(lone);

	return lone_lisp_buffer_copy(lone, actual, &actual->as.text.bytes,
			text, length, LONE_LISP_TAG_TEXT);
}

struct lone_lisp_value lone_lisp_text_from_c_string(struct lone_lisp *lone, char *c_string)
{
	return lone_lisp_text_transfer(lone, (unsigned char *) c_string, lone_c_string_length(c_string), false);
}

struct lone_lisp_value lone_lisp_text_to_symbol(struct lone_lisp *lone, struct lone_lisp_value text)
{
	return lone_lisp_intern_text(lone, text);
}
