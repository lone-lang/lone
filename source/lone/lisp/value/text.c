/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/value.h>
#include <lone/lisp/value/text.h>
#include <lone/lisp/value/bytes.h>
#include <lone/lisp/value/symbol.h>

#include <lone/memory/functions.h>

struct lone_lisp_value lone_lisp_text_transfer(struct lone_lisp *lone,
		unsigned char *text, size_t length, bool should_deallocate)
{
	struct lone_lisp_value value = lone_lisp_bytes_transfer(lone, text, length, should_deallocate);
	value.as.heap_value->type = LONE_LISP_TYPE_TEXT;
	return value;
}

struct lone_lisp_value lone_lisp_text_transfer_bytes(struct lone_lisp *lone,
		struct lone_bytes bytes, bool should_deallocate)
{
	return lone_lisp_text_transfer(lone, bytes.pointer, bytes.count, should_deallocate);
}

struct lone_lisp_value lone_lisp_text_copy(struct lone_lisp *lone, unsigned char *text, size_t length)
{
	struct lone_lisp_value value = lone_lisp_bytes_copy(lone, text, length);
	value.as.heap_value->type = LONE_LISP_TYPE_TEXT;
	return value;
}

struct lone_lisp_value lone_lisp_text_from_c_string(struct lone_lisp *lone, char *c_string)
{
	return lone_lisp_text_transfer(lone, (unsigned char *) c_string, lone_c_string_length(c_string), false);
}

struct lone_lisp_value lone_lisp_text_to_symbol(struct lone_lisp *lone, struct lone_lisp_value text)
{
	return lone_lisp_intern_text(lone, text);
}
