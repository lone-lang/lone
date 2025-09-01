/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/types.h>

#include <lone/lisp/value/vector.h>

#include <lone/linux.h>

struct lone_lisp_value lone_lisp_nil(void)
{
	return (struct lone_lisp_value) { .tagged = LONE_LISP_TYPE_NIL };
}

struct lone_lisp_value lone_lisp_false(void)
{
	return (struct lone_lisp_value) { .tagged = LONE_LISP_TYPE_FALSE };
}

struct lone_lisp_value lone_lisp_true(void)
{
	return (struct lone_lisp_value) { .tagged = LONE_LISP_TYPE_TRUE };
}

struct lone_lisp_value lone_lisp_boolean_for(bool value)
{
	if (value) {
		return lone_lisp_true();
	} else {
		return lone_lisp_false();
	}
}

enum lone_lisp_value_type lone_lisp_type_of(struct lone_lisp_value value)
{
	return value.tagged & 7;
}

struct lone_lisp_heap_value *lone_lisp_heap_value_of(struct lone_lisp_value value)
{
	return (struct lone_lisp_heap_value *) value.tagged;
}

lone_lisp_integer lone_lisp_integer_of(struct lone_lisp_value value)
{
	return value.tagged >> 3;
}

bool lone_lisp_has_same_type(struct lone_lisp_value x, struct lone_lisp_value y)
{
	if (lone_lisp_type_of(x) == lone_lisp_type_of(y)) {
		if (lone_lisp_is_heap_value(x) && lone_lisp_is_heap_value(y)) {
			if (lone_lisp_heap_value_of(x)->type == lone_lisp_heap_value_of(y)->type) {
				return true;
			} else {
				return false;
			}
		} else {
			return true;
		}
	} else {
		return false;
	}
}

bool lone_lisp_is_register_value(struct lone_lisp_value value)
{
	return lone_lisp_type_of(value) != LONE_LISP_TYPE_HEAP_VALUE;
}

bool lone_lisp_is_register_value_of_type(struct lone_lisp_value value, enum lone_lisp_value_type register_value_type)
{
	return lone_lisp_is_register_value(value) && lone_lisp_type_of(value) == register_value_type;
}

bool lone_lisp_is_heap_value(struct lone_lisp_value value)
{
	return lone_lisp_type_of(value) == LONE_LISP_TYPE_HEAP_VALUE;
}

bool lone_lisp_is_heap_value_of_type(struct lone_lisp_value value, enum lone_lisp_heap_value_type heap_value_type)
{
	return lone_lisp_is_heap_value(value) && lone_lisp_heap_value_of(value)->type == heap_value_type;
}

bool lone_lisp_is_integer(struct lone_lisp_value value)
{
	return lone_lisp_is_register_value_of_type(value, LONE_LISP_TYPE_INTEGER);
}

bool lone_lisp_is_nil(struct lone_lisp_value value)
{
	return lone_lisp_is_register_value_of_type(value, LONE_LISP_TYPE_NIL);
}

bool lone_lisp_is_false(struct lone_lisp_value value)
{
	return lone_lisp_is_register_value_of_type(value, LONE_LISP_TYPE_FALSE);
}

bool lone_lisp_is_true(struct lone_lisp_value value)
{
	return lone_lisp_is_register_value_of_type(value, LONE_LISP_TYPE_TRUE);
}

bool lone_lisp_is_falsy(struct lone_lisp_value value)
{
	return lone_lisp_is_nil(value) || lone_lisp_is_false(value);
}

bool lone_lisp_is_truthy(struct lone_lisp_value value)
{
	return !lone_lisp_is_falsy(value);
}

bool lone_lisp_is_module(struct lone_lisp_value value)
{
	return lone_lisp_is_heap_value_of_type(value, LONE_LISP_TYPE_MODULE);
}

bool lone_lisp_is_function(struct lone_lisp_value value)
{
	return lone_lisp_is_heap_value_of_type(value, LONE_LISP_TYPE_FUNCTION);
}

bool lone_lisp_is_primitive(struct lone_lisp_value value)
{
	return lone_lisp_is_heap_value_of_type(value, LONE_LISP_TYPE_PRIMITIVE);
}

bool lone_lisp_is_applicable(struct lone_lisp_value value)
{
	return lone_lisp_is_function(value) || lone_lisp_is_primitive(value);
}

bool lone_lisp_is_list(struct lone_lisp_value value)
{
	return lone_lisp_is_heap_value_of_type(value, LONE_LISP_TYPE_LIST);
}

bool lone_lisp_is_list_or_nil(struct lone_lisp_value value)
{
	return lone_lisp_is_nil(value) || lone_lisp_is_list(value);
}

bool lone_lisp_is_vector(struct lone_lisp_value value)
{
	return lone_lisp_is_heap_value_of_type(value, LONE_LISP_TYPE_VECTOR);
}

bool lone_lisp_is_table(struct lone_lisp_value value)
{
	return lone_lisp_is_heap_value_of_type(value, LONE_LISP_TYPE_TABLE);
}

bool lone_lisp_has_bytes(struct lone_lisp_value value)
{
	return lone_lisp_is_bytes(value) || lone_lisp_is_text(value) || lone_lisp_is_symbol(value);
}

bool lone_lisp_is_bytes(struct lone_lisp_value value)
{
	return lone_lisp_is_heap_value_of_type(value, LONE_LISP_TYPE_BYTES);
}

bool lone_lisp_is_text(struct lone_lisp_value value)
{
	return lone_lisp_is_heap_value_of_type(value, LONE_LISP_TYPE_TEXT);
}

bool lone_lisp_is_symbol(struct lone_lisp_value value)
{
	return lone_lisp_is_heap_value_of_type(value, LONE_LISP_TYPE_SYMBOL);
}

bool lone_lisp_is_identical(struct lone_lisp_value x, struct lone_lisp_value y)
{
	if (lone_lisp_has_same_type(x, y)) {
		switch (lone_lisp_type_of(x)) {
		case LONE_LISP_TYPE_NIL:
		case LONE_LISP_TYPE_FALSE:
		case LONE_LISP_TYPE_TRUE:
			return true;
		case LONE_LISP_TYPE_HEAP_VALUE:
			return lone_lisp_heap_value_of(x) == lone_lisp_heap_value_of(y);
		case LONE_LISP_TYPE_INTEGER:
			return lone_lisp_integer_of(x) == lone_lisp_integer_of(y);
		}
	} else {
		return false;
	}
}

bool lone_lisp_is_equivalent(struct lone_lisp_value x, struct lone_lisp_value y)
{
	if (!lone_lisp_has_same_type(x, y)) { return false; }

	switch (lone_lisp_type_of(x)) {
	case LONE_LISP_TYPE_NIL:
	case LONE_LISP_TYPE_FALSE:
	case LONE_LISP_TYPE_TRUE:
	case LONE_LISP_TYPE_INTEGER:
		return lone_lisp_is_identical(x, y);
	case LONE_LISP_TYPE_HEAP_VALUE:
		break;
	}

	switch (lone_lisp_heap_value_of(x)->type) {
	case LONE_LISP_TYPE_TEXT:
	case LONE_LISP_TYPE_BYTES:
		return lone_bytes_is_equal(lone_lisp_heap_value_of(x)->as.bytes,
		                           lone_lisp_heap_value_of(y)->as.bytes);

	case LONE_LISP_TYPE_MODULE:
	case LONE_LISP_TYPE_FUNCTION:
	case LONE_LISP_TYPE_PRIMITIVE:
	case LONE_LISP_TYPE_LIST:
	case LONE_LISP_TYPE_VECTOR:
	case LONE_LISP_TYPE_TABLE:
	case LONE_LISP_TYPE_SYMBOL:
		return lone_lisp_is_identical(x, y);
	}
}

static bool lone_lisp_list_is_equal(struct lone_lisp_value x, struct lone_lisp_value y)
{
	return lone_lisp_is_equal(lone_lisp_heap_value_of(x)->as.list.first,
	                          lone_lisp_heap_value_of(y)->as.list.first)
	       &&
	       lone_lisp_is_equal(lone_lisp_heap_value_of(x)->as.list.rest,
		                  lone_lisp_heap_value_of(y)->as.list.rest);
}

static bool lone_lisp_vector_is_equal(struct lone_lisp_value x, struct lone_lisp_value y)
{
	struct lone_lisp_value current;
	size_t i;

	if (lone_lisp_vector_count(x) != lone_lisp_vector_count(y)) return false;

	LONE_LISP_VECTOR_FOR_EACH(current, x, i) {
		if (!lone_lisp_is_equal(current, lone_lisp_vector_get_value_at(y, i))) {
			return false;
		}
	}

	return true;
}

static bool lone_lisp_table_is_equal(struct lone_lisp_value x, struct lone_lisp_value y)
{
	return lone_lisp_is_identical(x, y);
}

bool lone_lisp_is_equal(struct lone_lisp_value x, struct lone_lisp_value y)
{
	if (!lone_lisp_has_same_type(x, y)) { return false; }

	switch (lone_lisp_type_of(x)) {
	case LONE_LISP_TYPE_NIL:
	case LONE_LISP_TYPE_FALSE:
	case LONE_LISP_TYPE_TRUE:
	case LONE_LISP_TYPE_INTEGER:
		return lone_lisp_is_identical(x, y);
	case LONE_LISP_TYPE_HEAP_VALUE:
		break;
	}

	switch (lone_lisp_heap_value_of(x)->type) {
	case LONE_LISP_TYPE_LIST:
		return lone_lisp_list_is_equal(x, y);
	case LONE_LISP_TYPE_VECTOR:
		return lone_lisp_vector_is_equal(x, y);
	case LONE_LISP_TYPE_TABLE:
		return lone_lisp_table_is_equal(x, y);

	case LONE_LISP_TYPE_SYMBOL:
		return lone_lisp_is_identical(x, y);

	case LONE_LISP_TYPE_MODULE:
	case LONE_LISP_TYPE_FUNCTION:
	case LONE_LISP_TYPE_PRIMITIVE:
	case LONE_LISP_TYPE_TEXT:
	case LONE_LISP_TYPE_BYTES:
		return lone_lisp_is_equivalent(x, y);
	}
}

static bool integers(struct lone_lisp_value x, struct lone_lisp_value y)
{
	return lone_lisp_is_integer(x) && lone_lisp_is_integer(y);
}

bool lone_lisp_integer_is_less_than(struct lone_lisp_value x, struct lone_lisp_value y)
{
	if (integers(x, y)) {
		return lone_lisp_integer_of(x) < lone_lisp_integer_of(y);
	} else {
		/* can't compare incompatible or non-integers integers */ linux_exit(-1);
	}
}

bool lone_lisp_integer_is_less_than_or_equal_to(struct lone_lisp_value x, struct lone_lisp_value y)
{
	if (integers(x, y)) {
		return lone_lisp_integer_of(x) <= lone_lisp_integer_of(y);
	} else {
		/* can't compare incompatible or non-integers integers */ linux_exit(-1);
	}
}

bool lone_lisp_integer_is_greater_than(struct lone_lisp_value x, struct lone_lisp_value y)
{
	if (integers(x, y)) {
		return lone_lisp_integer_of(x) > lone_lisp_integer_of(y);
	} else {
		/* can't compare incompatible or non-integers integers */ linux_exit(-1);
	}
}

bool lone_lisp_integer_is_greater_than_or_equal_to(struct lone_lisp_value x, struct lone_lisp_value y)
{
	if (integers(x, y)) {
		return lone_lisp_integer_of(x) >= lone_lisp_integer_of(y);
	} else {
		/* can't compare incompatible or non-integers integers */ linux_exit(-1);
	}
}
