#include <lone/linux.h>
#include <lone/value.h>
#include <lone/value/list.h>
#include <lone/value/vector.h>
#include <lone/memory/allocator.h>
#include <lone/memory/functions.h>

#include <lone/struct/lisp.h>
#include <lone/struct/value.h>
#include <lone/struct/list.h>

struct lone_value *lone_list_create(struct lone_lisp *lone, struct lone_value *first, struct lone_value *rest)
{
	struct lone_value *value = lone_value_create(lone);
	value->type = LONE_LIST;
	value->list.first = first;
	value->list.rest = rest;
	return value;
}

struct lone_value *lone_list_create_nil(struct lone_lisp *lone)
{
	return lone_list_create(lone, 0, 0);
}

struct lone_value *lone_nil(struct lone_lisp *lone)
{
	return lone->constants.nil;
}

struct lone_value *lone_list_first(struct lone_value *value)
{
	return value->list.first;
}

struct lone_value *lone_list_rest(struct lone_value *value)
{
	return value->list.rest;
}

struct lone_value *lone_list_set_first(struct lone_value *list, struct lone_value *value)
{
	return list->list.first = value;
}

struct lone_value *lone_list_set_rest(struct lone_value *list, struct lone_value *rest)
{
	return list->list.rest = rest;
}

struct lone_value *lone_list_append(struct lone_lisp *lone, struct lone_value *list, struct lone_value *value)
{
	lone_list_set_first(list, value);
	return lone_list_set_rest(list, lone_list_create_nil(lone));
}

struct lone_value *lone_list_build(struct lone_lisp *lone, size_t count, ...)
{
	struct lone_value *list = lone_list_create_nil(lone), *head = list, *argument;
	va_list arguments;
	size_t i;

	va_start(arguments, count);

	for (i = 0; i < count; ++i) {
		argument = va_arg(arguments, struct lone_value *);
		head = lone_list_append(lone, head, argument);
	}

	va_end(arguments);

	return list;
}

struct lone_value *lone_list_to_vector(struct lone_lisp *lone, struct lone_value *list)
{
	struct lone_value *vector = lone_vector_create(lone, 16), *head;

	for (head = list; !lone_is_nil(head); head = lone_list_rest(head)) {
		lone_vector_push(lone, vector, lone_list_first(head));
	}

	return vector;
}

struct lone_value *lone_list_flatten(struct lone_lisp *lone, struct lone_value *list)
{
	struct lone_value *flattened = lone_list_create_nil(lone), *head, *flat_head, *return_head, *first;

	for (head = list, flat_head = flattened; !lone_is_nil(head); head = lone_list_rest(head)) {
		first = lone_list_first(head);

		if (lone_is_list(first)) {
			return_head = lone_list_flatten(lone, first);

			for (/* return_head */; !lone_is_nil(return_head); return_head = lone_list_rest(return_head)) {
				flat_head = lone_list_append(lone, flat_head, lone_list_first(return_head));
			}

		} else {
			flat_head = lone_list_append(lone, flat_head, first);
		}
	}

	return flattened;
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
