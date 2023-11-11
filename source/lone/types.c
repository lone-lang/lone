/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/types.h>
#include <lone/linux.h>
#include <lone/memory/functions.h>

#include <lone/struct/value.h>

bool lone_has_same_type(struct lone_value *x, struct lone_value *y)
{
	return x->type == y->type;
}

bool lone_is_module(struct lone_value *value)
{
	return value->type == LONE_MODULE;
}

bool lone_is_function(struct lone_value *value)
{
	return value->type == LONE_FUNCTION;
}

bool lone_is_primitive(struct lone_value *value)
{
	return value->type == LONE_PRIMITIVE;
}

bool lone_is_applicable(struct lone_value *value)
{
	return lone_is_function(value) || lone_is_primitive(value);
}

bool lone_is_list(struct lone_value *value)
{
	return value->type == LONE_LIST;
}

bool lone_is_vector(struct lone_value *value)
{
	return value->type == LONE_VECTOR;
}

bool lone_is_table(struct lone_value *value)
{
	return value->type == LONE_TABLE;
}

bool lone_is_nil(struct lone_value *value)
{
	return lone_is_list(value) && value->list.first == 0 && value->list.rest == 0;
}

bool lone_has_bytes(struct lone_value *value)
{
	return value->type == LONE_TEXT || value->type == LONE_SYMBOL || value->type == LONE_BYTES;
}

bool lone_is_bytes(struct lone_value *value)
{
	return value->type == LONE_BYTES;
}

bool lone_is_text(struct lone_value *value)
{
	return value->type == LONE_TEXT;
}

bool lone_is_symbol(struct lone_value *value)
{
	return value->type == LONE_SYMBOL;
}

bool lone_is_integer(struct lone_value *value)
{
	return value->type == LONE_INTEGER;
}

bool lone_is_pointer(struct lone_value *value)
{
	return value->type == LONE_POINTER;
}

bool lone_is_identical(struct lone_value *x, struct lone_value *y)
{
	return x == y;
}

bool lone_is_equivalent(struct lone_value *x, struct lone_value *y)
{
	if (lone_is_identical(x, y)) { return true; }
	if (!lone_has_same_type(x, y)) { return false; }

	switch (x->type) {
	case LONE_SYMBOL:
	case LONE_TEXT:
	case LONE_BYTES:
		return lone_bytes_equals(x->bytes, y->bytes);
	case LONE_INTEGER:
		return x->integer == y->integer;
	case LONE_POINTER:
		return x->pointer.address == y->pointer.address;

	case LONE_MODULE: case LONE_FUNCTION: case LONE_PRIMITIVE:
	case LONE_LIST: case LONE_VECTOR: case LONE_TABLE:
		return lone_is_identical(x, y);
	}
}

bool lone_list_is_equal(struct lone_value *x, struct lone_value *y)
{
	return lone_is_equal(x->list.first, y->list.first) && lone_is_equal(x->list.rest, y->list.rest);
}

bool lone_vector_is_equal(struct lone_value *x, struct lone_value *y)
{
	size_t i;

	if (x->vector.count != y->vector.count) return false;

	for (i = 0; i < x->vector.count; ++i) {
		if (!lone_is_equal(x->vector.values[i], y->vector.values[i])) {
			return false;
		}
	}

	return true;
}

bool lone_table_is_equal(struct lone_value *x, struct lone_value *y)
{
	return lone_is_identical(x, y);
}

bool lone_is_equal(struct lone_value *x, struct lone_value *y)
{
	if (lone_is_identical(x, y)) { return true; }
	if (!lone_has_same_type(x, y)) { return false; }

	switch (x->type) {
	case LONE_LIST:
		return lone_list_is_equal(x, y);
	case LONE_VECTOR:
		return lone_vector_is_equal(x, y);
	case LONE_TABLE:
		return lone_table_is_equal(x, y);

	case LONE_MODULE: case LONE_FUNCTION: case LONE_PRIMITIVE:
	case LONE_SYMBOL: case LONE_TEXT: case LONE_BYTES:
	case LONE_INTEGER: case LONE_POINTER:
		return lone_is_equivalent(x, y);
	}
}

bool lone_integer_is_less_than(struct lone_value *x, struct lone_value *y)
{
	if (!(lone_is_integer(x) && lone_is_integer(y))) { /* can't compare non-integers */ linux_exit(-1); }

	if (x->integer < y->integer) { return true; }
	else { return false; }
}

bool lone_integer_is_less_than_or_equal_to(struct lone_value *x, struct lone_value *y)
{
	if (!(lone_is_integer(x) && lone_is_integer(y))) { /* can't compare non-integers */ linux_exit(-1); }

	if (x->integer <= y->integer) { return true; }
	else { return false; }
}

bool lone_integer_is_greater_than(struct lone_value *x, struct lone_value *y)
{
	if (!(lone_is_integer(x) && lone_is_integer(y))) { /* can't compare non-integers */ linux_exit(-1); }

	if (x->integer > y->integer) { return true; }
	else { return false; }
}

bool lone_integer_is_greater_than_or_equal_to(struct lone_value *x, struct lone_value *y)
{
	if (!(lone_is_integer(x) && lone_is_integer(y))) { /* can't compare non-integers */ linux_exit(-1); }

	if (x->integer >= y->integer) { return true; }
	else { return false; }
}

bool lone_bytes_equals(struct lone_bytes x, struct lone_bytes y)
{
	if (x.count != y.count) return false;
	for (size_t i = 0; i < x.count; ++i) if (x.pointer[i] != y.pointer[i]) return false;
	return true;
}

bool lone_bytes_equals_c_string(struct lone_bytes bytes, char *c_string)
{
	struct lone_bytes c_string_bytes = { lone_c_string_length(c_string), (unsigned char *) c_string };
	return lone_bytes_equals(bytes, c_string_bytes);
}
