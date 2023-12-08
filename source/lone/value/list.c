/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/linux.h>
#include <lone/value.h>
#include <lone/value/list.h>
#include <lone/value/vector.h>
#include <lone/memory/heap.h>
#include <lone/memory/allocator.h>
#include <lone/memory/functions.h>

struct lone_value lone_list_create(struct lone_lisp *lone, struct lone_value first, struct lone_value rest)
{
	struct lone_heap_value *actual = lone_heap_allocate_value(lone);
	actual->type = LONE_LIST;
	actual->as.list.first = first;
	actual->as.list.rest = rest;
	return lone_value_from_heap_value(actual);
}

struct lone_value lone_list_create_nil(struct lone_lisp *lone)
{
	return lone_list_create(lone, lone_nil(), lone_nil());
}

struct lone_value lone_list_first(struct lone_value value)
{
	if (lone_is_nil(value)) {
		return lone_nil();
	} else if (!lone_is_list(value)) {
		/* expected a list value */ linux_exit(-1);
	} else {
		return value.as.heap_value->as.list.first;
	}
}

struct lone_value lone_list_rest(struct lone_value value)
{
	if (lone_is_nil(value)) {
		return lone_nil();
	} else if (!lone_is_list(value)) {
		/* expected a list value */ linux_exit(-1);
	} else {
		return value.as.heap_value->as.list.rest;
	}
}

struct lone_value lone_list_set_first(struct lone_lisp *lone, struct lone_value value, struct lone_value first)
{
	if (lone_is_nil(value)) {
		return lone_list_create(lone, first, lone_nil());
	} else if (!lone_is_list(value)) {
		/* expected a list value */ linux_exit(-1);
	} else {
		return value.as.heap_value->as.list.first = first;
	}
}

struct lone_value lone_list_set_rest(struct lone_lisp *lone, struct lone_value value, struct lone_value rest)
{
	if (lone_is_nil(value)) {
		return lone_list_create(lone, lone_nil(), rest);
	} else if (!lone_is_list(value)) {
		/* expected a list value */ linux_exit(-1);
	} else {
		return value.as.heap_value->as.list.rest = rest;
	}
}

struct lone_value lone_list_append(struct lone_lisp *lone, struct lone_value *first, struct lone_value *head, struct lone_value value)
{
	struct lone_value new = lone_list_create(lone, value, lone_nil());

	if (lone_is_nil(*head)) {
		*first = *head = new;
	} else {
		*head = lone_list_set_rest(lone, *head, new);
	}

	return new;
}

struct lone_value lone_list_build(struct lone_lisp *lone, size_t count, ...)
{
	struct lone_value list, head;
	va_list arguments;
	size_t i;

	va_start(arguments, count);

	for (i = 0, list = head = lone_nil(); i < count; ++i) {
		lone_list_append(lone, &list, &head, *va_arg(arguments, struct lone_value *));
	}

	va_end(arguments);

	return list;
}

struct lone_value lone_list_to_vector(struct lone_lisp *lone, struct lone_value list)
{
	struct lone_value vector;

	vector = lone_vector_create(lone, 16);

	for (/* list */; !lone_is_nil(list); list = lone_list_rest(list)) {
		lone_vector_push(lone, vector, lone_list_first(list));
	}

	return vector;
}

struct lone_value lone_list_flatten(struct lone_lisp *lone, struct lone_value list)
{
	struct lone_value flattened, head, flat_head, return_head, first;

	for (head = list, flattened = flat_head = lone_nil(); !lone_is_nil(head); head = lone_list_rest(head)) {
		first = lone_list_first(head);

		if (lone_is_list(first)) {
			return_head = lone_list_flatten(lone, first);

			for (/* return_head */; !lone_is_nil(return_head); return_head = lone_list_rest(return_head)) {
				lone_list_append(lone, &flattened, &flat_head, lone_list_first(return_head));
			}

		} else {
			lone_list_append(lone, &flattened, &flat_head, first);
		}
	}

	return flattened;
}

bool lone_list_has_rest(struct lone_value value)
{
	if (lone_is_nil(value)) {
		return false;
	} else if (!lone_is_list(value)) {
		/* expected a list value */ linux_exit(-1);
	} else {
		return !lone_is_nil(value.as.heap_value->as.list.rest);
	}
}

bool lone_list_destructure(struct lone_value list, size_t count, ...)
{
	struct lone_value *current;
	va_list arguments;
	size_t i;

	va_start(arguments, count);

	if (lone_is_nil(list)) { /* empty list, no values to extract */ return false; }
	if (!lone_is_list(list)) { /* expected a list */ linux_exit(-1); }

	i = 0;
	while (!lone_is_nil(list)) {
		*va_arg(arguments, struct lone_value *) = lone_list_first(list);
		++i;

		if (i < count) {
			/* expect more values */
			if (!lone_list_has_rest(list)) { /* less values than expected */ return true; }
		} else {
			/* expect end of list */
			if (lone_list_has_rest(list)) { /* more values than expected */ return true; }
		}

		list = lone_list_rest(list);
	}

	va_end(arguments);

	return false;
}
