/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/utilities.h>

#include <lone/lisp/value/list.h>

#include <lone/memory/allocator.h>
#include <lone/memory/functions.h>

#include <lone/linux.h>

struct lone_lisp_value lone_lisp_apply_predicate(struct lone_lisp *lone,
		struct lone_lisp_value arguments, lone_lisp_predicate_function function)
{
	if (lone_lisp_is_nil(arguments) || !lone_lisp_is_nil(lone_lisp_list_rest(arguments))) {
		/* predicates accept exactly one argument */ linux_exit(-1);
	}

	return function(lone_lisp_list_first(arguments))? lone_lisp_true() : lone_lisp_false();
}

struct lone_lisp_value lone_lisp_apply_comparator(struct lone_lisp *lone,
		struct lone_lisp_value arguments, lone_lisp_comparator_function function)
{
	struct lone_lisp_value current, next;

	if (lone_lisp_is_nil(arguments)) { return lone_lisp_true(); }

	while (1) {
		if (!lone_lisp_list_has_rest(arguments)) { break; }
		current = lone_lisp_list_first(arguments);
		arguments = lone_lisp_list_rest(arguments);
		next = lone_lisp_list_first(arguments);

		if (!function(current, next)) { return lone_lisp_false(); }
	}

	return lone_lisp_true();
}

struct lone_bytes lone_lisp_join(struct lone_lisp *lone,
		struct lone_lisp_value separator, struct lone_lisp_value arguments,
		lone_lisp_predicate_function is_valid)
{
	struct lone_lisp_value head, argument;
	unsigned char *joined, *from, *to;
	size_t total, position, count;

	if (!is_valid) { is_valid = lone_lisp_has_bytes; }
	if (is_valid != lone_lisp_has_bytes && is_valid != lone_lisp_is_bytes &&
	    is_valid != lone_lisp_is_text   && is_valid != lone_lisp_is_symbol) {
		/* invalid predicate function */ linux_exit(-1);
	}

	if (!lone_lisp_is_nil(separator)) {
		if (!is_valid(separator)) { linux_exit(-1); }
	}

	total = 0;
	position = 0;

	for (head = arguments; !lone_lisp_is_nil(head); head = lone_lisp_list_rest(head)) {
		argument = lone_lisp_list_first(head);

		if (!is_valid(argument)) { linux_exit(-1); }

		total += lone_lisp_heap_value_of(argument)->as.bytes.count;

		if (!lone_lisp_is_nil(separator) && !lone_lisp_is_nil(lone_lisp_list_rest(head))) {
			total += lone_lisp_heap_value_of(separator)->as.bytes.count;
		}
	}

	joined = lone_allocate_uninitialized(lone->system, total + 1);

	for (head = arguments; !lone_lisp_is_nil(head); head = lone_lisp_list_rest(head)) {
		argument = lone_lisp_list_first(head);

		count = lone_lisp_heap_value_of(argument)->as.bytes.count;
		from = lone_lisp_heap_value_of(argument)->as.bytes.pointer;
		to = joined + position;
		lone_memory_move(from, to, count);
		position += count;

		if (!lone_lisp_is_nil(separator) && !lone_lisp_is_nil(lone_lisp_list_rest(head))) {
			count = lone_lisp_heap_value_of(separator)->as.bytes.count;
			from = lone_lisp_heap_value_of(separator)->as.bytes.pointer;
			to = joined + position;
			lone_memory_move(from, to, count);
			position += count;
		}
	}

	joined[total] = '\0';

	return (struct lone_bytes) { .count = total, .pointer = joined };
}

struct lone_bytes lone_lisp_concatenate(struct lone_lisp *lone,
		struct lone_lisp_value arguments, lone_lisp_predicate_function is_valid)
{
	return lone_lisp_join(lone, lone_lisp_nil(), arguments, is_valid);
}
