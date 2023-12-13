/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/definitions.h>
#include <lone/types.h>
#include <lone/utilities.h>
#include <lone/lisp/constants.h>

#include <lone/memory/allocator.h>
#include <lone/memory/functions.h>

#include <lone/value/list.h>

#include <lone/linux.h>

struct lone_value lone_apply_predicate(struct lone_lisp *lone, struct lone_value arguments, lone_predicate function)
{
	if (lone_is_nil(arguments) || !lone_is_nil(lone_list_rest(arguments))) { /* predicates accept exactly one argument */ linux_exit(-1); }
	return function(lone_list_first(arguments)) ? lone_true(lone) : lone_nil();
}

struct lone_value lone_apply_comparator(struct lone_lisp *lone, struct lone_value arguments, lone_comparator function)
{
	struct lone_value current, next;

	if (lone_is_nil(arguments)) { return lone_true(lone); }

	while (1) {
		if (!lone_list_has_rest(arguments)) { break; }
		current = lone_list_first(arguments);
		arguments = lone_list_rest(arguments);
		next = lone_list_first(arguments);

		if (!function(current, next)) { return lone_nil(); }
	}

	return lone_true(lone);
}

struct lone_bytes lone_join(struct lone_lisp *lone, struct lone_value separator, struct lone_value arguments, lone_predicate is_valid)
{
	struct lone_value head, argument;
	unsigned char *joined, *from, *to;
	size_t total, position, count;

	if (!is_valid) { is_valid = lone_has_bytes; }
	if (is_valid != lone_has_bytes && is_valid != lone_is_bytes &&
	    is_valid != lone_is_text   && is_valid != lone_is_symbol) {
		/* invalid predicate function */ linux_exit(-1);
	}

	if (!lone_is_nil(separator)) {
		if (!is_valid(separator)) { linux_exit(-1); }
	}

	total = 0;
	position = 0;

	for (head = arguments; !lone_is_nil(head); head = lone_list_rest(head)) {
		argument = lone_list_first(head);

		if (!is_valid(argument)) { linux_exit(-1); }

		total += argument.as.heap_value->as.bytes.count;

		if (!lone_is_nil(separator) && !lone_is_nil(lone_list_rest(head))) {
			total += separator.as.heap_value->as.bytes.count;
		}
	}

	joined = lone_allocate(lone, total + 1);

	for (head = arguments; !lone_is_nil(head); head = lone_list_rest(head)) {
		argument = lone_list_first(head);

		count = argument.as.heap_value->as.bytes.count;
		from = argument.as.heap_value->as.bytes.pointer;
		to = joined + position;
		lone_memory_move(from, to, count);
		position += count;

		if (!lone_is_nil(separator) && !lone_is_nil(lone_list_rest(head))) {
			count = separator.as.heap_value->as.bytes.count;
			from = separator.as.heap_value->as.bytes.pointer;
			to = joined + position;
			lone_memory_move(from, to, count);
			position += count;
		}
	}

	joined[total] = '\0';

	return (struct lone_bytes) { .count = total, .pointer = joined };
}

struct lone_bytes lone_concatenate(struct lone_lisp *lone, struct lone_value arguments, lone_predicate is_valid)
{
	return lone_join(lone, lone_nil(), arguments, is_valid);
}

long lone_min(long x, long y)
{
	if (x <= y) { return x; } else { return y; }
}

long lone_max(long x, long y)
{
	if (x >= y) { return x; } else { return y; }
}
