/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/types.h>
#include <lone/lisp/heap.h>

#include <lone/linux.h>

struct lone_lisp_value lone_lisp_nil(void)
{
	return (struct lone_lisp_value) { .tagged = LONE_LISP_TAG_NIL };
}

struct lone_lisp_value lone_lisp_false(void)
{
	return (struct lone_lisp_value) { .tagged = LONE_LISP_TAG_FALSE };
}

struct lone_lisp_value lone_lisp_true(void)
{
	return (struct lone_lisp_value) { .tagged = LONE_LISP_TAG_TRUE };
}

struct lone_lisp_value lone_lisp_boolean_for(bool value)
{
	if (value) {
		return lone_lisp_true();
	} else {
		return lone_lisp_false();
	}
}

enum lone_lisp_tag lone_lisp_type_of(struct lone_lisp_value value)
{
	enum lone_lisp_tag tag = value.tagged & LONE_LISP_TAG_MASK;

	/* normalize inline tags to their canonical heap type */
	if ((tag & LONE_LISP_INLINE_TYPE_MASK) == LONE_LISP_INLINE_TYPE_SYMBOL) {
		return LONE_LISP_TAG_SYMBOL;
	}

	if ((tag & LONE_LISP_INLINE_TYPE_MASK) == LONE_LISP_INLINE_TYPE_TEXT) {
		return LONE_LISP_TAG_TEXT;
	}

	return tag;
}

struct lone_lisp_heap_value *lone_lisp_heap_value_of(struct lone_lisp *lone, struct lone_lisp_value value)
{
	size_t index;

	index = ((unsigned long) value.tagged) >> LONE_LISP_INDEX_SHIFT;

	return &lone->heap.values[index];
}

lone_lisp_integer lone_lisp_integer_of(struct lone_lisp_value value)
{
	return value.tagged >> LONE_LISP_DATA_SHIFT;
}

bool lone_lisp_has_same_type(struct lone_lisp *lone, struct lone_lisp_value x, struct lone_lisp_value y)
{
	(void) lone;
	return lone_lisp_type_of(x) == lone_lisp_type_of(y);
}

bool lone_lisp_is_register_value(struct lone_lisp_value value)
{
	return value.tagged & 1;
}

bool lone_lisp_is_heap_value(struct lone_lisp_value value)
{
	return !lone_lisp_is_register_value(value);
}

bool lone_lisp_is_integer(struct lone_lisp *lone, struct lone_lisp_value value)
{
	(void) lone;
	return (value.tagged & LONE_LISP_TAG_MASK) == LONE_LISP_TAG_INTEGER;
}

bool lone_lisp_is_nil(struct lone_lisp_value value)
{
	return (value.tagged & LONE_LISP_TAG_MASK) == LONE_LISP_TAG_NIL;
}

bool lone_lisp_is_false(struct lone_lisp_value value)
{
	return (value.tagged & LONE_LISP_TAG_MASK) == LONE_LISP_TAG_FALSE;
}

bool lone_lisp_is_true(struct lone_lisp_value value)
{
	return (value.tagged & LONE_LISP_TAG_MASK) == LONE_LISP_TAG_TRUE;
}

bool lone_lisp_is_falsy(struct lone_lisp_value value)
{
	return lone_lisp_is_nil(value) || lone_lisp_is_false(value);
}

bool lone_lisp_is_truthy(struct lone_lisp_value value)
{
	return !lone_lisp_is_falsy(value);
}

bool lone_lisp_is_module(struct lone_lisp *lone, struct lone_lisp_value value)
{
	(void) lone;
	return (value.tagged & LONE_LISP_TAG_MASK) == LONE_LISP_TAG_MODULE;
}

bool lone_lisp_is_function(struct lone_lisp *lone, struct lone_lisp_value value)
{
	(void) lone;
	return (value.tagged & LONE_LISP_TAG_MASK) == LONE_LISP_TAG_FUNCTION;
}

bool lone_lisp_is_primitive(struct lone_lisp *lone, struct lone_lisp_value value)
{
	(void) lone;
	return (value.tagged & LONE_LISP_TAG_MASK) == LONE_LISP_TAG_PRIMITIVE;
}

bool lone_lisp_is_applicable(struct lone_lisp *lone, struct lone_lisp_value value)
{
	(void) lone;
	return (value.tagged & LONE_LISP_TAG_MASK) == LONE_LISP_TAG_FUNCTION
	    || (value.tagged & LONE_LISP_TAG_MASK) == LONE_LISP_TAG_PRIMITIVE;
}

bool lone_lisp_is_list(struct lone_lisp *lone, struct lone_lisp_value value)
{
	(void) lone;
	return (value.tagged & LONE_LISP_TAG_MASK) == LONE_LISP_TAG_NIL
	    || (value.tagged & LONE_LISP_TAG_MASK) == LONE_LISP_TAG_LIST;
}

bool lone_lisp_is_vector(struct lone_lisp *lone, struct lone_lisp_value value)
{
	(void) lone;
	return (value.tagged & LONE_LISP_TAG_MASK) == LONE_LISP_TAG_VECTOR;
}

bool lone_lisp_is_table(struct lone_lisp *lone, struct lone_lisp_value value)
{
	(void) lone;
	return (value.tagged & LONE_LISP_TAG_MASK) == LONE_LISP_TAG_TABLE;
}

bool lone_lisp_has_bytes(struct lone_lisp *lone, struct lone_lisp_value value)
{
	(void) lone;
	return (value.tagged & LONE_LISP_TAG_MASK) == LONE_LISP_TAG_BYTES
	    || (value.tagged & LONE_LISP_TAG_MASK) == LONE_LISP_TAG_TEXT
	    || (value.tagged & LONE_LISP_TAG_MASK) == LONE_LISP_TAG_SYMBOL
	    || lone_lisp_is_inline_symbol(value)
	    || lone_lisp_is_inline_text(value);
}

bool lone_lisp_is_bytes(struct lone_lisp *lone, struct lone_lisp_value value)
{
	(void) lone;
	return (value.tagged & LONE_LISP_TAG_MASK) == LONE_LISP_TAG_BYTES;
}

bool lone_lisp_is_text(struct lone_lisp *lone, struct lone_lisp_value value)
{
	(void) lone;
	return (value.tagged & LONE_LISP_TAG_MASK) == LONE_LISP_TAG_TEXT
	    || lone_lisp_is_inline_text(value);
}

bool lone_lisp_is_inline_symbol(struct lone_lisp_value value)
{
	return (value.tagged & LONE_LISP_INLINE_TYPE_MASK) == LONE_LISP_INLINE_TYPE_SYMBOL;
}

bool lone_lisp_is_inline_text(struct lone_lisp_value value)
{
	return (value.tagged & LONE_LISP_INLINE_TYPE_MASK) == LONE_LISP_INLINE_TYPE_TEXT;
}

bool lone_lisp_is_inline_value(struct lone_lisp_value value)
{
	return (value.tagged & 0x81) == 0x81;
}

bool lone_lisp_is_symbol(struct lone_lisp *lone, struct lone_lisp_value value)
{
	(void) lone;
	return (value.tagged & LONE_LISP_TAG_MASK) == LONE_LISP_TAG_SYMBOL
	    || lone_lisp_is_inline_symbol(value);
}

struct lone_bytes lone_lisp_inline_value_bytes(struct lone_lisp_value *value)
{
	size_t count = (value->tagged >> LONE_LISP_INLINE_LENGTH_SHIFT) & LONE_LISP_INLINE_LENGTH_MASK;
	unsigned char *pointer = ((unsigned char *) &value->tagged) + 1;
	return (struct lone_bytes) { .count = count, .pointer = pointer };
}

static struct lone_lisp_value lone_lisp_inline_create(unsigned char type, unsigned char *bytes, size_t count)
{
	long tagged;
	size_t i;

	tagged = type | (long) (count << LONE_LISP_INLINE_LENGTH_SHIFT);

	for (i = 0; i < count; ++i) {
		tagged |= ((long) bytes[i]) << (LONE_LISP_DATA_SHIFT + i * 8);
	}

	return (struct lone_lisp_value) { .tagged = tagged };
}

struct lone_lisp_value lone_lisp_inline_symbol_create(unsigned char *bytes, size_t count)
{
	return lone_lisp_inline_create(LONE_LISP_INLINE_TYPE_SYMBOL, bytes, count);
}

struct lone_lisp_value lone_lisp_inline_text_create(unsigned char *bytes, size_t count)
{
	return lone_lisp_inline_create(LONE_LISP_INLINE_TYPE_TEXT, bytes, count);
}

struct lone_bytes lone_lisp_symbol_name(struct lone_lisp *lone, struct lone_lisp_value *value)
{
	if (lone_lisp_is_inline_symbol(*value)) {
		return lone_lisp_inline_value_bytes(value);
	} else {
		return lone_lisp_heap_value_of(lone, *value)->as.symbol.name;
	}
}

bool lone_lisp_is_identical(struct lone_lisp *lone, struct lone_lisp_value x, struct lone_lisp_value y)
{
	(void) lone;
	return x.tagged == y.tagged;
}

static struct lone_bytes lone_lisp_text_or_bytes_data(struct lone_lisp *lone, struct lone_lisp_value *value)
{
	if (lone_lisp_is_inline_value(*value)) {
		return lone_lisp_inline_value_bytes(value);
	}
	return lone_lisp_heap_value_of(lone, *value)->as.bytes;
}

bool lone_lisp_is_equivalent(struct lone_lisp *lone, struct lone_lisp_value x, struct lone_lisp_value y)
{
	if (!lone_lisp_has_same_type(lone, x, y)) { return false; }

	switch (lone_lisp_type_of(x)) {
	case LONE_LISP_TAG_NIL:
	case LONE_LISP_TAG_FALSE:
	case LONE_LISP_TAG_TRUE:
	case LONE_LISP_TAG_INTEGER:
		return x.tagged == y.tagged;
	case LONE_LISP_TAG_TEXT:
	case LONE_LISP_TAG_BYTES:
		return lone_bytes_is_equal(lone_lisp_text_or_bytes_data(lone, &x),
		                           lone_lisp_text_or_bytes_data(lone, &y));
	case LONE_LISP_TAG_MODULE:
	case LONE_LISP_TAG_FUNCTION:
	case LONE_LISP_TAG_PRIMITIVE:
	case LONE_LISP_TAG_CONTINUATION:
	case LONE_LISP_TAG_GENERATOR:
	case LONE_LISP_TAG_LIST:
	case LONE_LISP_TAG_VECTOR:
	case LONE_LISP_TAG_TABLE:
	case LONE_LISP_TAG_SYMBOL:
		return x.tagged == y.tagged;
	default:
		linux_exit(-1);
	}
}

static bool lone_lisp_list_is_equal(struct lone_lisp *lone, struct lone_lisp_value x, struct lone_lisp_value y)
{
	return lone_lisp_is_equal(lone,
	                          lone_lisp_heap_value_of(lone, x)->as.list.first,
	                          lone_lisp_heap_value_of(lone, y)->as.list.first)
	       &&
	       lone_lisp_is_equal(lone,
	                          lone_lisp_heap_value_of(lone, x)->as.list.rest,
	                          lone_lisp_heap_value_of(lone, y)->as.list.rest);
}

static bool lone_lisp_vector_is_equal(struct lone_lisp *lone, struct lone_lisp_value x, struct lone_lisp_value y)
{
	struct lone_lisp_value current;
	size_t i;

	if (lone_lisp_vector_count(lone, x) != lone_lisp_vector_count(lone, y)) return false;

	LONE_LISP_VECTOR_FOR_EACH(lone, current, x, i) {
		if (!lone_lisp_is_equal(lone, current, lone_lisp_vector_get_value_at(lone, y, i))) {
			return false;
		}
	}

	return true;
}

static bool lone_lisp_table_is_equal(struct lone_lisp *lone, struct lone_lisp_value x, struct lone_lisp_value y)
{
	return lone_lisp_is_identical(lone, x, y);
}

bool lone_lisp_is_equal(struct lone_lisp *lone, struct lone_lisp_value x, struct lone_lisp_value y)
{
	if (x.tagged == y.tagged) { return true; }
	if (!lone_lisp_has_same_type(lone, x, y)) { return false; }

	switch (lone_lisp_type_of(x)) {
	case LONE_LISP_TAG_NIL:
	case LONE_LISP_TAG_FALSE:
	case LONE_LISP_TAG_TRUE:
	case LONE_LISP_TAG_INTEGER:
	case LONE_LISP_TAG_SYMBOL:
		return x.tagged == y.tagged;
	case LONE_LISP_TAG_LIST:
		return lone_lisp_list_is_equal(lone, x, y);
	case LONE_LISP_TAG_VECTOR:
		return lone_lisp_vector_is_equal(lone, x, y);
	case LONE_LISP_TAG_TABLE:
		return lone_lisp_table_is_equal(lone, x, y);
	case LONE_LISP_TAG_MODULE:
	case LONE_LISP_TAG_FUNCTION:
	case LONE_LISP_TAG_PRIMITIVE:
	case LONE_LISP_TAG_CONTINUATION:
	case LONE_LISP_TAG_GENERATOR:
	case LONE_LISP_TAG_TEXT:
	case LONE_LISP_TAG_BYTES:
		return lone_lisp_is_equivalent(lone, x, y);
	default:
		linux_exit(-1);
	}
}

static bool integers(struct lone_lisp *lone, struct lone_lisp_value x, struct lone_lisp_value y)
{
	return lone_lisp_is_integer(lone, x) && lone_lisp_is_integer(lone, y);
}

bool lone_lisp_integer_is_less_than(struct lone_lisp *lone, struct lone_lisp_value x, struct lone_lisp_value y)
{
	if (integers(lone, x, y)) {
		return lone_lisp_integer_of(x) < lone_lisp_integer_of(y);
	} else {
		/* can't compare incompatible or non-integers integers */ linux_exit(-1);
	}
}

bool lone_lisp_integer_is_less_than_or_equal_to(struct lone_lisp *lone, struct lone_lisp_value x, struct lone_lisp_value y)
{
	if (integers(lone, x, y)) {
		return lone_lisp_integer_of(x) <= lone_lisp_integer_of(y);
	} else {
		/* can't compare incompatible or non-integers integers */ linux_exit(-1);
	}
}

bool lone_lisp_integer_is_greater_than(struct lone_lisp *lone, struct lone_lisp_value x, struct lone_lisp_value y)
{
	if (integers(lone, x, y)) {
		return lone_lisp_integer_of(x) > lone_lisp_integer_of(y);
	} else {
		/* can't compare incompatible or non-integers integers */ linux_exit(-1);
	}
}

bool lone_lisp_integer_is_greater_than_or_equal_to(struct lone_lisp *lone, struct lone_lisp_value x, struct lone_lisp_value y)
{
	if (integers(lone, x, y)) {
		return lone_lisp_integer_of(x) >= lone_lisp_integer_of(y);
	} else {
		/* can't compare incompatible or non-integers integers */ linux_exit(-1);
	}
}

struct lone_lisp_value lone_lisp_value_from_heap_value(struct lone_lisp *lone,
		struct lone_lisp_heap_value *heap_value, enum lone_lisp_tag tag)
{
	size_t index;

	index = (size_t) (heap_value - lone->heap.values);

	heap_value->type = tag;

	return (struct lone_lisp_value) {
		.tagged = (long) (((unsigned long) index << LONE_LISP_INDEX_SHIFT) | tag),
	};
}

struct lone_lisp_value lone_lisp_retag(struct lone_lisp_value value, enum lone_lisp_tag new_tag)
{
	/* replace the tag byte, keep index and metadata */
	value.tagged = (value.tagged & ~((long) LONE_LISP_TAG_MASK)) | new_tag;

	return value;
}

struct lone_lisp_value lone_lisp_retag_frame(struct lone_lisp_machine_stack_frame frame, enum lone_lisp_tag new_tag)
{
	return lone_lisp_retag((struct lone_lisp_value) { .tagged = frame.tagged }, new_tag);
}
