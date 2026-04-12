/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/utilities.h>

#include <lone/memory/allocator.h>
#include <lone/memory/functions.h>

#include <lone/linux.h>

struct lone_lisp_value lone_lisp_apply_predicate(struct lone_lisp *lone,
		struct lone_lisp_value arguments, lone_lisp_predicate_function function)
{
	if (lone_lisp_is_nil(arguments) || !lone_lisp_is_nil(lone_lisp_list_rest(lone, arguments))) {
		/* predicates accept exactly one argument */ linux_exit(-1);
	}

	return function(lone, lone_lisp_list_first(lone, arguments))? lone_lisp_true() : lone_lisp_false();
}

struct lone_lisp_value lone_lisp_apply_comparator(struct lone_lisp *lone,
		struct lone_lisp_value arguments, lone_lisp_comparator_function function)
{
	struct lone_lisp_value current, next;

	if (lone_lisp_is_nil(arguments)) { return lone_lisp_true(); }

	while (1) {
		if (!lone_lisp_list_has_rest(lone, arguments)) { break; }
		current = lone_lisp_list_first(lone, arguments);
		arguments = lone_lisp_list_rest(lone, arguments);
		next = lone_lisp_list_first(lone, arguments);

		if (!function(lone, current, next)) { return lone_lisp_false(); }
	}

	return lone_lisp_true();
}

static struct lone_bytes select_bytes(struct lone_lisp *lone, struct lone_lisp_value *value)
{
	if (lone_lisp_is_symbol(lone, *value)) {
		return lone_lisp_symbol_name(lone, value);
	}
	if (lone_lisp_is_inline_value(*value)) {
		return lone_lisp_inline_value_bytes(value);
	}
	return lone_lisp_heap_value_of(lone, *value)->as.bytes;
}

struct lone_bytes lone_lisp_join(struct lone_lisp *lone,
		struct lone_lisp_value separator, struct lone_lisp_value arguments,
		lone_lisp_predicate_function is_valid)
{
	struct lone_lisp_value head, argument;
	unsigned char *joined, *from, *to;
	size_t total, position, count, separator_count, allocation_size;

	if (!is_valid) { is_valid = lone_lisp_has_bytes; }
	if (is_valid != lone_lisp_has_bytes && is_valid != lone_lisp_is_bytes &&
	    is_valid != lone_lisp_is_text   && is_valid != lone_lisp_is_symbol) {
		/* invalid predicate function */ linux_exit(-1);
	}

	if (!lone_lisp_is_nil(separator)) {
		if (!is_valid(lone, separator)) { linux_exit(-1); }
	}

	total = 0;
	position = 0;
	separator_count = select_bytes(lone, &separator).count;

	for (head = arguments; !lone_lisp_is_nil(head); head = lone_lisp_list_rest(lone, head)) {
		argument = lone_lisp_list_first(lone, head);

		if (!is_valid(lone, argument)) { linux_exit(-1); }

		if (__builtin_add_overflow(total,select_bytes(lone, &argument).count, &total)) {
			goto overflow;
		}

		if (!lone_lisp_is_nil(separator) && !lone_lisp_is_nil(lone_lisp_list_rest(lone, head))) {
			if (__builtin_add_overflow(total, separator_count, &total)) {
				goto overflow;
			}
		}
	}

	if (__builtin_add_overflow(total, (size_t) 1, &allocation_size)) {
		goto overflow;
	}

	joined =
		lone_memory_allocate(
			lone->system,
			allocation_size,
			1,
			1,
			LONE_MEMORY_ALLOCATION_FLAGS_NONE
		);

	for (head = arguments; !lone_lisp_is_nil(head); head = lone_lisp_list_rest(lone, head)) {
		argument = lone_lisp_list_first(lone, head);

		count = select_bytes(lone, &argument).count;
		from  = select_bytes(lone, &argument).pointer;
		to = joined + position;
		lone_memory_move(from, to, count);
		position += count;

		if (!lone_lisp_is_nil(separator) && !lone_lisp_is_nil(lone_lisp_list_rest(lone, head))) {
			count = select_bytes(lone, &separator).count;
			from = select_bytes(lone, &separator).pointer;
			to = joined + position;
			lone_memory_move(from, to, count);
			position += count;
		}
	}

	joined[total] = '\0';

	return (struct lone_bytes) { .count = total, .pointer = joined };

overflow:
	linux_exit(-1);
}

struct lone_bytes lone_lisp_concatenate(struct lone_lisp *lone,
		struct lone_lisp_value arguments, lone_lisp_predicate_function is_valid)
{
	return lone_lisp_join(lone, lone_lisp_nil(), arguments, is_valid);
}
