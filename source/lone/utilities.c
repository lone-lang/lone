/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/definitions.h>
#include <lone/types.h>
#include <lone/utilities.h>
#include <lone/lisp/constants.h>

#include <lone/memory/allocator.h>
#include <lone/memory/functions.h>

#include <lone/value/list.h>

#include <lone/linux.h>

struct lone_value *lone_apply_predicate(struct lone_lisp *lone, struct lone_value *arguments, lone_predicate function)
{
	if (lone_is_nil(arguments) || !lone_is_nil(lone_list_rest(arguments))) { /* predicates accept exactly one argument */ linux_exit(-1); }
	return function(lone_list_first(arguments)) ? lone_true(lone) : lone_nil(lone);
}

struct lone_value *lone_apply_comparator(struct lone_lisp *lone, struct lone_value *arguments, lone_comparator function)
{
	struct lone_value *argument, *next;

	while (1) {
		if (lone_is_nil(arguments)) { break; }
		argument = lone_list_first(arguments);
		arguments = lone_list_rest(arguments);
		next = lone_list_first(arguments);

		if (next && !function(argument, next)) { return lone_nil(lone); }
	}

	return lone_true(lone);
}

struct lone_bytes lone_join(struct lone_lisp *lone, struct lone_value *separator, struct lone_value *arguments, lone_predicate is_valid)
{
	struct lone_value *head, *argument;
	unsigned char *joined;
	size_t total = 0, position = 0;

	if (!is_valid) { is_valid = lone_has_bytes; }
	if (is_valid != lone_has_bytes && is_valid != lone_is_bytes &&
	    is_valid != lone_is_text   && is_valid != lone_is_symbol) {
		/* invalid predicate function */ linux_exit(-1);
	}

	if (separator && !lone_is_nil(separator)) {
		if (!is_valid(separator)) { linux_exit(-1); }
	}

	for (head = arguments; head && !lone_is_nil(head); head = lone_list_rest(head)) {
		argument = lone_list_first(head);
		if (!is_valid(argument)) { linux_exit(-1); }

		total += argument->bytes.count;
		if (separator && !lone_is_nil(separator)) {
			if (!lone_is_nil(lone_list_rest(head))) { total += separator->bytes.count; }
		}
	}

	joined = lone_allocate(lone, total + 1);

	for (head = arguments; head && !lone_is_nil(head); head = lone_list_rest(head)) {
		argument = lone_list_first(head);

		lone_memory_move(argument->bytes.pointer, joined + position, argument->bytes.count);
		position += argument->bytes.count;

		if (separator && !lone_is_nil(separator)) {
			if (!lone_is_nil(lone_list_rest(head))) {
				lone_memory_move(separator->bytes.pointer, joined + position, separator->bytes.count);
				position += separator->bytes.count;
			}
		}
	}

	joined[total] = '\0';

	return (struct lone_bytes) { .count = total, .pointer = joined };
}

struct lone_bytes lone_concatenate(struct lone_lisp *lone, struct lone_value *arguments, lone_predicate is_valid)
{
	return lone_join(lone, 0, arguments, is_valid);
}

struct lone_auxiliary_value lone_auxiliary_vector_value(struct lone_auxiliary_vector *auxiliary, long type)
{
	for (/* auxiliary */; auxiliary->type != AT_NULL; ++auxiliary) {
		if (auxiliary->type == type) {
			return auxiliary->value;
		}
	}

	return (struct lone_auxiliary_value) { .as.integer = 0 };
}

size_t lone_auxiliary_vector_page_size(struct lone_auxiliary_vector *auxiliary)
{
	return lone_auxiliary_vector_value(auxiliary, AT_PAGESZ).as.integer;
}

struct lone_bytes lone_auxiliary_vector_random(struct lone_auxiliary_vector *auxiliary)
{
	struct lone_bytes random = { 0, 0 };

	random.pointer = lone_auxiliary_vector_value(auxiliary, AT_RANDOM).as.pointer;
	random.count = 16;

	return random;
}

struct lone_elf_segments lone_auxiliary_vector_elf_segments(struct lone_auxiliary_vector *auxiliary)
{
	return (struct lone_elf_segments) {
		.entry_size  = lone_auxiliary_vector_value(auxiliary, AT_PHENT).as.unsigned_integer,
		.entry_count = lone_auxiliary_vector_value(auxiliary, AT_PHNUM).as.unsigned_integer,
		.segments    = lone_auxiliary_vector_value(auxiliary, AT_PHDR).as.pointer
	};
}
