/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/modules/intrinsic/text.h>
#include <lone/lisp/modules/intrinsic/lone.h>

#include <lone/lisp/machine.h>
#include <lone/lisp/machine/stack.h>
#include <lone/lisp/module.h>
#include <lone/lisp/utilities.h>
#include <lone/lisp/heap.h>

#include <lone/unicode.h>

#include <lone/linux.h>

void lone_lisp_modules_intrinsic_text_initialize(struct lone_lisp *lone)
{
	struct lone_lisp_value name, module;
	struct lone_lisp_function_flags flags;

	name = lone_lisp_intern_c_string(lone, "text");
	module = lone_lisp_module_for_name(lone, name);
	flags.evaluate_arguments = true;
	flags.evaluate_result = false;

	lone_lisp_module_export_primitive(lone, module, "to-symbol",
			"text_to_symbol", lone_lisp_primitive_text_to_symbol, module, flags);

	lone_lisp_module_export_primitive(lone, module, "join",
			"text_join", lone_lisp_primitive_text_join, module, flags);

	lone_lisp_module_export_primitive(lone, module, "concatenate",
			"text_concatenate", lone_lisp_primitive_text_concatenate, module, flags);

	lone_lisp_module_export_primitive(lone, module, "code-point-count",
			"text_code_point_count", lone_lisp_primitive_text_code_point_count, module, flags);

	lone_lisp_module_export_primitive(lone, module, "code-unit-count",
			"text_code_unit_count", lone_lisp_primitive_text_code_unit_count, module, flags);

	lone_lisp_module_export_primitive(lone, module, "code-point-at",
			"text_code_point_at", lone_lisp_primitive_text_code_point_at, module, flags);

	lone_lisp_module_export_primitive(lone, module, "from-code-point",
			"text_from_code_point", lone_lisp_primitive_text_from_code_point, module, flags);

	lone_lisp_module_export_primitive(lone, module, "slice",
			"text_slice", lone_lisp_primitive_text_slice, module, flags);
}

LONE_LISP_PRIMITIVE(text_to_symbol)
{
	struct lone_lisp_value arguments, text, symbol;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	if (lone_lisp_list_destructure(lone, arguments, 1, &text)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_lisp_is_text(lone, text)) {
		/* argument not a text value: (to-symbol 123) */ linux_exit(-1);
	}

	symbol = lone_lisp_text_to_symbol(lone, text);

	lone_lisp_machine_push_value(lone, machine, symbol);
	return 0;
}

LONE_LISP_PRIMITIVE(text_join)
{
	struct lone_lisp_value arguments, text;
	struct lone_bytes joined;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	joined = lone_lisp_join(lone,
			lone_lisp_list_first(lone, arguments),
			lone_lisp_list_rest(lone, arguments),
			lone_lisp_is_text);

	text = lone_lisp_text_transfer_bytes(lone, joined, true);

	lone_lisp_machine_push_value(lone, machine, text);
	return 0;
}

LONE_LISP_PRIMITIVE(text_concatenate)
{
	struct lone_lisp_value arguments, text;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	text = lone_lisp_text_transfer_bytes(lone, lone_lisp_concatenate(lone, arguments, lone_lisp_is_text), true);

	lone_lisp_machine_push_value(lone, machine, text);
	return 0;
}

static size_t lone_lisp_text_code_point_count_of(struct lone_lisp *lone, struct lone_lisp_value text)
{
	struct lone_lisp_heap_value *heap_value;
	struct lone_unicode_utf8_validation_result validation;
	struct lone_bytes bytes;

	if (lone_lisp_is_inline_text(text)) {
		bytes = lone_lisp_inline_value_bytes(&text);
		validation = lone_unicode_utf8_validate(bytes);
		return validation.code_point_count;
	}

	heap_value = lone_lisp_heap_value_of(lone, text);

	if (heap_value->code_point_count_cached) {
		return heap_value->as.text.code_point_count;
	}

	validation = lone_unicode_utf8_validate(heap_value->as.text.bytes);
	heap_value->as.text.code_point_count = validation.code_point_count;
	heap_value->code_point_count_cached = true;

	return validation.code_point_count;
}

LONE_LISP_PRIMITIVE(text_code_point_count)
{
	struct lone_lisp_value arguments, text, count;

	switch (step) {
	case 0:
		arguments = lone_lisp_machine_pop_value(lone, machine);

		if (lone_lisp_list_destructure(lone, arguments, 1, &text)) {
			/* wrong number of arguments */ linux_exit(-1);
		}

	validate:
		if (!lone_lisp_is_text(lone, text)) {
			return lone_lisp_signal_emit(lone, machine, 1,
					lone_lisp_intern_c_string(lone, "type-error"), text);
		}

		count = lone_lisp_integer_create(lone_lisp_text_code_point_count_of(lone, text));

		lone_lisp_machine_push_value(lone, machine, count);
		return 0;

	case 1:
		text = machine->value;
		goto validate;
	}

	linux_exit(-1);
}
LONE_LISP_PRIMITIVE(text_code_unit_count)
{
	struct lone_lisp_value arguments, text, count;
	struct lone_bytes bytes;

	switch (step) {
	case 0:
		arguments = lone_lisp_machine_pop_value(lone, machine);

		if (lone_lisp_list_destructure(lone, arguments, 1, &text)) {
			/* wrong number of arguments */ linux_exit(-1);
		}

	validate:
		if (!lone_lisp_is_text(lone, text)) {
			return lone_lisp_signal_emit(lone, machine, 1,
					lone_lisp_intern_c_string(lone, "type-error"), text);
		}

		bytes = lone_lisp_bytes_of(lone, &text);
		count = lone_lisp_integer_create(bytes.count);

		lone_lisp_machine_push_value(lone, machine, count);
		return 0;

	case 1:
		text = machine->value;
		goto validate;
	}

	linux_exit(-1);
}

LONE_LISP_PRIMITIVE(text_code_point_at)
{
	struct lone_lisp_value arguments, text, index_value;
	struct lone_unicode_utf8_decode_result decoded;
	struct lone_bytes bytes;
	lone_lisp_integer index;
	size_t code_point_count, i;

	switch (step) {
	case 0:
		arguments = lone_lisp_machine_pop_value(lone, machine);

		if (lone_lisp_list_destructure(lone, arguments, 2, &text, &index_value)) {
			/* wrong number of arguments */ linux_exit(-1);
		}

	validate_text:
		if (!lone_lisp_is_text(lone, text)) {
			lone_lisp_machine_push_value(lone, machine, index_value);
			return lone_lisp_signal_emit(lone, machine, 1,
					lone_lisp_intern_c_string(lone, "type-error"), text);
		}

	validate_index:
		if (!lone_lisp_is_integer(lone, index_value)) {
			lone_lisp_machine_push_value(lone, machine, text);
			return lone_lisp_signal_emit(lone, machine, 2,
					lone_lisp_intern_c_string(lone, "type-error"), index_value);
		}

		index = lone_lisp_integer_of(index_value);

		if (index < 0) {
			lone_lisp_machine_push_value(lone, machine, text);
			return lone_lisp_signal_emit(lone, machine, 2,
					lone_lisp_intern_c_string(lone, "index-error"), index_value);
		}

		code_point_count = lone_lisp_text_code_point_count_of(lone, text);
		if ((size_t) index >= code_point_count) {
			lone_lisp_machine_push_value(lone, machine, text);
			return lone_lisp_signal_emit(lone, machine, 2,
					lone_lisp_intern_c_string(lone, "index-error"), index_value);
		}

		bytes = lone_lisp_bytes_of(lone, &text);

		for (i = 0; i < (size_t) index; ++i) {
			decoded = lone_unicode_utf8_decode(bytes);
			bytes.pointer += decoded.bytes_read;
			bytes.count   -= decoded.bytes_read;
		}

		decoded = lone_unicode_utf8_decode(bytes);

		lone_lisp_machine_push_value(lone, machine,
				lone_lisp_integer_create(decoded.code_point));
		return 0;

	case 1:
		text = machine->value;
		index_value = lone_lisp_machine_pop_value(lone, machine);
		goto validate_text;

	case 2:
		index_value = machine->value;
		text = lone_lisp_machine_pop_value(lone, machine);
		goto validate_index;
	}

	linux_exit(-1);
}

LONE_LISP_PRIMITIVE(text_from_code_point)
{
	struct lone_lisp_value arguments, integer;
	struct lone_unicode_utf8_encode_result encoded;
	lone_lisp_integer code_point;

	switch (step) {
	case 0:
		arguments = lone_lisp_machine_pop_value(lone, machine);

		if (lone_lisp_list_destructure(lone, arguments, 1, &integer)) {
			/* wrong number of arguments */ linux_exit(-1);
		}

	validate:
		if (!lone_lisp_is_integer(lone, integer)) {
			return lone_lisp_signal_emit(lone, machine, 1,
					lone_lisp_intern_c_string(lone, "type-error"), integer);
		}

		code_point = lone_lisp_integer_of(integer);

		if (code_point < 0 || !lone_unicode_is_valid_code_point((lone_u32) code_point)) {
			return lone_lisp_signal_emit(lone, machine, 1,
					lone_lisp_intern_c_string(lone, "invalid-unicode"), integer);
		}

		encoded = lone_unicode_utf8_encode((lone_u32) code_point);

		lone_lisp_machine_push_value(lone, machine,
				lone_lisp_text_copy(lone, encoded.bytes, encoded.bytes_written));
		return 0;

	case 1:
		integer = machine->value;
		goto validate;
	}

	linux_exit(-1);
}

LONE_LISP_PRIMITIVE(text_slice)
{
	struct lone_lisp_value arguments, text, start_value, end_value;
	struct lone_unicode_utf8_decode_result decoded;
	struct lone_bytes bytes;
	unsigned char *slice_start;
	lone_lisp_integer start_index, end_index;
	size_t code_point_count, i, slice_length;
	bool has_end;

	switch (step) {
	case 0:
		arguments = lone_lisp_machine_pop_value(lone, machine);

		if (lone_lisp_is_nil(arguments)) { /* no arguments */ linux_exit(-1); }

		text = lone_lisp_list_first(lone, arguments);
		arguments = lone_lisp_list_rest(lone, arguments);

		if (lone_lisp_is_nil(arguments)) { /* no start index */ linux_exit(-1); }

		start_value = lone_lisp_list_first(lone, arguments);
		arguments = lone_lisp_list_rest(lone, arguments);

		has_end = !lone_lisp_is_nil(arguments);

		if (has_end) {
			end_value = lone_lisp_list_first(lone, arguments);
			arguments = lone_lisp_list_rest(lone, arguments);

			if (!lone_lisp_is_nil(arguments)) {
				/* too many arguments */ linux_exit(-1);
			}
		}

	validate_text:
		if (!lone_lisp_is_text(lone, text)) {
			lone_lisp_machine_push_value(lone, machine, start_value);
			lone_lisp_machine_push_value(lone, machine, has_end ? end_value : lone_lisp_nil());
			return lone_lisp_signal_emit(lone, machine, 1,
					lone_lisp_intern_c_string(lone, "type-error"), text);
		}

	validate_start:
		if (!lone_lisp_is_integer(lone, start_value)) {
			lone_lisp_machine_push_value(lone, machine, text);
			lone_lisp_machine_push_value(lone, machine, has_end ? end_value : lone_lisp_nil());
			return lone_lisp_signal_emit(lone, machine, 2,
					lone_lisp_intern_c_string(lone, "type-error"), start_value);
		}

		start_index = lone_lisp_integer_of(start_value);
		code_point_count = lone_lisp_text_code_point_count_of(lone, text);

		if (has_end) {
		validate_end:
			if (!lone_lisp_is_integer(lone, end_value)) {
				lone_lisp_machine_push_value(lone, machine, text);
				lone_lisp_machine_push_value(lone, machine, start_value);
				return lone_lisp_signal_emit(lone, machine, 3,
						lone_lisp_intern_c_string(lone, "type-error"), end_value);
			}

			end_index = lone_lisp_integer_of(end_value);
		} else {
			end_index = (lone_lisp_integer) code_point_count;
		}

		if (start_index < 0 || (size_t) start_index > code_point_count) {
			lone_lisp_machine_push_value(lone, machine, text);
			lone_lisp_machine_push_value(lone, machine, has_end ? end_value : lone_lisp_nil());
			return lone_lisp_signal_emit(lone, machine, 2,
					lone_lisp_intern_c_string(lone, "index-error"), start_value);
		}

		if (end_index < 0 || (size_t) end_index > code_point_count) {
			lone_lisp_machine_push_value(lone, machine, text);
			lone_lisp_machine_push_value(lone, machine, start_value);
			return lone_lisp_signal_emit(lone, machine, 3,
					lone_lisp_intern_c_string(lone, "index-error"),
					lone_lisp_integer_create(end_index));
		}

		if (end_index < start_index) {
			lone_lisp_machine_push_value(lone, machine, text);
			lone_lisp_machine_push_value(lone, machine, start_value);
			return lone_lisp_signal_emit(lone, machine, 3,
					lone_lisp_intern_c_string(lone, "index-error"),
					lone_lisp_integer_create(end_index));
		}

		bytes = lone_lisp_bytes_of(lone, &text);

		for (i = 0; i < (size_t) start_index; ++i) {
			decoded = lone_unicode_utf8_decode(bytes);
			bytes.pointer += decoded.bytes_read;
			bytes.count   -= decoded.bytes_read;
		}

		slice_start = bytes.pointer;
		slice_length = 0;

		for (; i < (size_t) end_index; ++i) {
			decoded = lone_unicode_utf8_decode(bytes);
			bytes.pointer += decoded.bytes_read;
			bytes.count   -= decoded.bytes_read;
			slice_length  += decoded.bytes_read;
		}

		lone_lisp_machine_push_value(lone, machine,
				lone_lisp_text_copy(lone, slice_start, slice_length));
		return 0;

	case 1:
		text = machine->value;
		end_value = lone_lisp_machine_pop_value(lone, machine);
		has_end = !lone_lisp_is_nil(end_value);
		start_value = lone_lisp_machine_pop_value(lone, machine);
		goto validate_text;

	case 2:
		start_value = machine->value;
		end_value = lone_lisp_machine_pop_value(lone, machine);
		has_end = !lone_lisp_is_nil(end_value);
		text = lone_lisp_machine_pop_value(lone, machine);
		goto validate_start;

	case 3:
		end_value = machine->value;
		has_end = true;
		start_value = lone_lisp_machine_pop_value(lone, machine);
		text = lone_lisp_machine_pop_value(lone, machine);
		goto validate_end;
	}

	linux_exit(-1);
}
