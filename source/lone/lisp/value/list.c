/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/value.h>
#include <lone/lisp/value/list.h>
#include <lone/lisp/value/vector.h>

#include <lone/lisp/heap.h>

#include <lone/memory/allocator.h>
#include <lone/memory/functions.h>

#include <lone/linux.h>

struct lone_lisp_value lone_lisp_list_create(struct lone_lisp *lone,
		struct lone_lisp_value first, struct lone_lisp_value rest)
{
	struct lone_lisp_heap_value *actual = lone_lisp_heap_allocate_value(lone);
	actual->type = LONE_LISP_TYPE_LIST;
	actual->as.list.first = first;
	actual->as.list.rest = rest;
	return lone_lisp_value_from_heap_value(actual);
}

struct lone_lisp_value lone_lisp_list_create_nil(struct lone_lisp *lone)
{
	return lone_lisp_list_create(lone, lone_lisp_nil(), lone_lisp_nil());
}

struct lone_lisp_value lone_lisp_list_first(struct lone_lisp_value value)
{
	if (lone_lisp_is_nil(value)) {
		return lone_lisp_nil();
	} else if (!lone_lisp_is_list(value)) {
		/* expected a list value */ linux_exit(-1);
	} else {
		return value.as.heap_value->as.list.first;
	}
}

struct lone_lisp_value lone_lisp_list_rest(struct lone_lisp_value value)
{
	if (lone_lisp_is_nil(value)) {
		return lone_lisp_nil();
	} else if (!lone_lisp_is_list(value)) {
		/* expected a list value */ linux_exit(-1);
	} else {
		return value.as.heap_value->as.list.rest;
	}
}

struct lone_lisp_value lone_lisp_list_set_first(struct lone_lisp *lone,
		struct lone_lisp_value value, struct lone_lisp_value first)
{
	if (lone_lisp_is_nil(value)) {
		return lone_lisp_list_create(lone, first, lone_lisp_nil());
	} else if (!lone_lisp_is_list(value)) {
		/* expected a list value */ linux_exit(-1);
	} else {
		return value.as.heap_value->as.list.first = first;
	}
}

struct lone_lisp_value lone_lisp_list_set_rest(struct lone_lisp *lone,
		struct lone_lisp_value value, struct lone_lisp_value rest)
{
	if (lone_lisp_is_nil(value)) {
		return lone_lisp_list_create(lone, lone_lisp_nil(), rest);
	} else if (!lone_lisp_is_list(value)) {
		/* expected a list value */ linux_exit(-1);
	} else {
		return value.as.heap_value->as.list.rest = rest;
	}
}

struct lone_lisp_value lone_lisp_list_append(struct lone_lisp *lone,
		struct lone_lisp_value *first, struct lone_lisp_value *head,
		struct lone_lisp_value value)
{
	struct lone_lisp_value new = lone_lisp_list_create(lone, value, lone_lisp_nil());

	if (lone_lisp_is_nil(*head)) {
		*first = *head = new;
	} else {
		*head = lone_lisp_list_set_rest(lone, *head, new);
	}

	return new;
}

struct lone_lisp_value lone_lisp_list_build(struct lone_lisp *lone, size_t count, ...)
{
	struct lone_lisp_value list, head;
	va_list arguments;
	size_t i;

	va_start(arguments, count);

	for (i = 0, list = head = lone_lisp_nil(); i < count; ++i) {
		lone_lisp_list_append(lone, &list, &head, *va_arg(arguments, struct lone_lisp_value *));
	}

	va_end(arguments);

	return list;
}

struct lone_lisp_value lone_lisp_list_to_vector(struct lone_lisp *lone, struct lone_lisp_value list)
{
	struct lone_lisp_value vector;

	vector = lone_lisp_vector_create(lone, 16);

	for (/* list */; !lone_lisp_is_nil(list); list = lone_lisp_list_rest(list)) {
		lone_lisp_vector_push(lone, vector, lone_lisp_list_first(list));
	}

	return vector;
}

struct lone_lisp_value lone_lisp_list_flatten(struct lone_lisp *lone, struct lone_lisp_value list)
{
	struct lone_lisp_value flattened, head, flat_head, return_head, first;

	for (head = list, flattened = flat_head = lone_lisp_nil(); !lone_lisp_is_nil(head); head = lone_lisp_list_rest(head)) {
		first = lone_lisp_list_first(head);

		if (lone_lisp_is_list(first)) {
			return_head = lone_lisp_list_flatten(lone, first);

			for (/* return_head */; !lone_lisp_is_nil(return_head); return_head = lone_lisp_list_rest(return_head)) {
				lone_lisp_list_append(lone, &flattened, &flat_head, lone_lisp_list_first(return_head));
			}

		} else {
			lone_lisp_list_append(lone, &flattened, &flat_head, first);
		}
	}

	return flattened;
}

bool lone_lisp_list_has_rest(struct lone_lisp_value value)
{
	if (lone_lisp_is_nil(value)) {
		return false;
	} else if (!lone_lisp_is_list(value)) {
		/* expected a list value */ linux_exit(-1);
	} else {
		return !lone_lisp_is_nil(value.as.heap_value->as.list.rest);
	}
}

bool lone_lisp_list_destructure(struct lone_lisp_value list, size_t count, ...)
{
	va_list arguments;
	size_t i;

	va_start(arguments, count);

	if (lone_lisp_is_nil(list)) { /* empty list, no values to extract */ return false; }
	if (!lone_lisp_is_list(list)) { /* expected a list */ linux_exit(-1); }

	i = 0;
	while (!lone_lisp_is_nil(list)) {
		*va_arg(arguments, struct lone_lisp_value *) = lone_lisp_list_first(list);
		++i;

		if (i < count) {
			/* expect more values */
			if (!lone_lisp_list_has_rest(list)) { /* less values than expected */ return true; }
		} else {
			/* expect end of list */
			if (lone_lisp_list_has_rest(list)) { /* more values than expected */ return true; }
		}

		list = lone_lisp_list_rest(list);
	}

	va_end(arguments);

	return false;
}
