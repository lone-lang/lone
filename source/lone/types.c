/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/types.h>
#include <lone/linux.h>
#include <lone/memory/functions.h>

#include <lone/value/vector.h>

struct lone_value lone_nil(void)
{
	return (struct lone_value) { .type = LONE_NIL };
}

bool lone_has_same_type(struct lone_value x, struct lone_value y)
{
	if (x.type == y.type) {
		if (lone_is_heap_value(x) && lone_is_heap_value(y)) {
			if (x.as.heap_value->type == y.as.heap_value->type) {
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

bool lone_is_register_value(struct lone_value value)
{
	return value.type != LONE_HEAP_VALUE;
}

bool lone_is_register_value_of_type(struct lone_value value, enum lone_value_type register_value_type)
{
	return lone_is_register_value(value) && value.type == register_value_type;
}

bool lone_is_heap_value(struct lone_value value)
{
	return value.type == LONE_HEAP_VALUE;
}

bool lone_is_heap_value_of_type(struct lone_value value, enum lone_heap_value_type heap_value_type)
{
	return lone_is_heap_value(value) && value.as.heap_value->type == heap_value_type;
}

bool lone_is_module(struct lone_value value)
{
	return lone_is_heap_value_of_type(value, LONE_MODULE);
}

bool lone_is_function(struct lone_value value)
{
	return lone_is_heap_value_of_type(value, LONE_FUNCTION);
}

bool lone_is_primitive(struct lone_value value)
{
	return lone_is_heap_value_of_type(value, LONE_PRIMITIVE);
}

bool lone_is_applicable(struct lone_value value)
{
	return lone_is_function(value) || lone_is_primitive(value);
}

bool lone_is_list(struct lone_value value)
{
	return lone_is_heap_value_of_type(value, LONE_LIST);
}

bool lone_is_list_or_nil(struct lone_value value)
{
	return lone_is_nil(value) || lone_is_list(value);
}

bool lone_is_vector(struct lone_value value)
{
	return lone_is_heap_value_of_type(value, LONE_VECTOR);
}

bool lone_is_table(struct lone_value value)
{
	return lone_is_heap_value_of_type(value, LONE_TABLE);
}

bool lone_has_bytes(struct lone_value value)
{
	return lone_is_bytes(value) || lone_is_text(value) || lone_is_symbol(value);
}

bool lone_is_bytes(struct lone_value value)
{
	return lone_is_heap_value_of_type(value, LONE_BYTES);
}

bool lone_is_text(struct lone_value value)
{
	return lone_is_heap_value_of_type(value, LONE_TEXT);
}

bool lone_is_symbol(struct lone_value value)
{
	return lone_is_heap_value_of_type(value, LONE_SYMBOL);
}

bool lone_is_nil(struct lone_value value)
{
	return lone_is_register_value_of_type(value, LONE_NIL);
}

bool lone_is_integer(struct lone_value value)
{
	return lone_is_register_value_of_type(value, LONE_INTEGER);
}

bool lone_is_pointer(struct lone_value value)
{
	return lone_is_register_value_of_type(value, LONE_POINTER);
}

bool lone_is_identical(struct lone_value x, struct lone_value y)
{
	if (lone_has_same_type(x, y)) {
		switch (x.type) {
		case LONE_NIL:
			return true;
		case LONE_HEAP_VALUE:
			return x.as.heap_value == y.as.heap_value;
		case LONE_INTEGER:
			return x.as.signed_integer == y.as.signed_integer;
		case LONE_POINTER:
			return x.as.pointer.to_void == y.as.pointer.to_void;
		}
	} else {
		return false;
	}
}

bool lone_is_equivalent(struct lone_value x, struct lone_value y)
{
	if (!lone_has_same_type(x, y)) { return false; }

	switch (x.type) {
	case LONE_NIL:
	case LONE_INTEGER:
	case LONE_POINTER:
		return lone_is_identical(x, y);
	case LONE_HEAP_VALUE:
		break;
	}

	switch (x.as.heap_value->type) {
	case LONE_TEXT:
	case LONE_BYTES:
		return lone_bytes_equals(x.as.heap_value->as.bytes, y.as.heap_value->as.bytes);

	case LONE_MODULE:
	case LONE_FUNCTION:
	case LONE_PRIMITIVE:
	case LONE_LIST:
	case LONE_VECTOR:
	case LONE_TABLE:
	case LONE_SYMBOL:
		return lone_is_identical(x, y);
	}
}

static bool lone_list_is_equal(struct lone_value x, struct lone_value y)
{
	return lone_is_equal(x.as.heap_value->as.list.first, y.as.heap_value->as.list.first) &&
	       lone_is_equal(x.as.heap_value->as.list.rest,  y.as.heap_value->as.list.rest);
}

static bool lone_vector_is_equal(struct lone_value x, struct lone_value y)
{
	struct lone_value current;
	size_t i;

	if (lone_vector_count(x) != lone_vector_count(y)) return false;

	LONE_VECTOR_FOR_EACH(current, x, i) {
		if (!lone_is_equal(current, lone_vector_get_value_at(y, i))) {
			return false;
		}
	}

	return true;
}

static bool lone_table_is_equal(struct lone_value x, struct lone_value y)
{
	return lone_is_identical(x, y);
}

bool lone_is_equal(struct lone_value x, struct lone_value y)
{
	if (!lone_has_same_type(x, y)) { return false; }

	switch (x.type) {
	case LONE_NIL:
	case LONE_INTEGER:
	case LONE_POINTER:
		return lone_is_identical(x, y);
	case LONE_HEAP_VALUE:
		break;
	}

	switch (x.as.heap_value->type) {
	case LONE_LIST:
		return lone_list_is_equal(x, y);
	case LONE_VECTOR:
		return lone_vector_is_equal(x, y);
	case LONE_TABLE:
		return lone_table_is_equal(x, y);

	case LONE_SYMBOL:
		return lone_is_identical(x, y);

	case LONE_MODULE:
	case LONE_FUNCTION:
	case LONE_PRIMITIVE:
	case LONE_TEXT:
	case LONE_BYTES:
		return lone_is_equivalent(x, y);
	}
}

static bool integers(struct lone_value x, struct lone_value y)
{
	return lone_is_integer(x) && lone_is_integer(y);
}

bool lone_integer_is_less_than(struct lone_value x, struct lone_value y)
{
	if (integers(x, y)) {
		return x.as.signed_integer < y.as.signed_integer;
	} else {
		/* can't compare incompatible or non-integers integers */ linux_exit(-1);
	}
}

bool lone_integer_is_less_than_or_equal_to(struct lone_value x, struct lone_value y)
{
	if (integers(x, y)) {
		return x.as.signed_integer <= y.as.signed_integer;
	} else {
		/* can't compare incompatible or non-integers integers */ linux_exit(-1);
	}
}

bool lone_integer_is_greater_than(struct lone_value x, struct lone_value y)
{
	if (integers(x, y)) {
		return x.as.signed_integer > y.as.signed_integer;
	} else {
		/* can't compare incompatible or non-integers integers */ linux_exit(-1);
	}
}

bool lone_integer_is_greater_than_or_equal_to(struct lone_value x, struct lone_value y)
{
	if (integers(x, y)) {
		return x.as.signed_integer >= y.as.signed_integer;
	} else {
		/* can't compare incompatible or non-integers integers */ linux_exit(-1);
	}
}

#define LONE_READER(type) \
lone_##type lone_##type##_read(void *address) \
{ \
	lone_##type *type = address; \
	return *type; \
}

#define LONE_WRITER(type) \
void lone_##type##_write(void *address, lone_##type value) \
{ \
	lone_##type *type = address; \
	*type = value; \
}

LONE_READER(u8)
LONE_READER(s8)
LONE_READER(u16)
LONE_READER(s16)
LONE_READER(u32)
LONE_READER(s32)
LONE_READER(u64)
LONE_READER(s64)

LONE_WRITER(u8)
LONE_WRITER(s8)
LONE_WRITER(u16)
LONE_WRITER(s16)
LONE_WRITER(u32)
LONE_WRITER(s32)
LONE_WRITER(u64)
LONE_WRITER(s64)

#undef LONE_READER
#undef LONE_WRITER

bool lone_bytes_equals(struct lone_bytes x, struct lone_bytes y)
{
	if (x.count != y.count) return false;
	return lone_memory_compare(x.pointer, y.pointer, x.count) == 0;
}

bool lone_bytes_equals_c_string(struct lone_bytes bytes, char *c_string)
{
	struct lone_bytes c_string_bytes = { lone_c_string_length(c_string), (unsigned char *) c_string };
	return lone_bytes_equals(bytes, c_string_bytes);
}

bool lone_bytes_contains_offset(struct lone_bytes bytes, lone_size offset)
{
	return offset >= 0 && offset < bytes.count;
}

bool lone_bytes_contains_block(struct lone_bytes bytes, lone_size offset, lone_size size)
{
	return offset >= 0 && (offset + size) <= bytes.count;
}

bool lone_bytes_contains_address(struct lone_bytes bytes, void *pointer)
{
	void *start = bytes.pointer, *end = bytes.pointer + bytes.count;
	return pointer >= start && pointer < end;
}

#define LONE_BYTES_READER(type) \
lone_##type lone_bytes_read_##type(struct lone_bytes bytes, lone_size offset) \
{ \
	return lone_##type##_read(bytes.pointer + offset); \
}

#define LONE_BYTES_WRITER(type) \
void lone_bytes_write_##type(struct lone_bytes bytes, lone_size offset, lone_##type type) \
{ \
	lone_##type##_write(bytes.pointer + offset, type); \
}

LONE_BYTES_READER(u8)
LONE_BYTES_READER(s8)
LONE_BYTES_READER(u16)
LONE_BYTES_READER(s16)
LONE_BYTES_READER(u32)
LONE_BYTES_READER(s32)
LONE_BYTES_READER(u64)
LONE_BYTES_READER(s64)

LONE_BYTES_WRITER(u8)
LONE_BYTES_WRITER(s8)
LONE_BYTES_WRITER(u16)
LONE_BYTES_WRITER(s16)
LONE_BYTES_WRITER(u32)
LONE_BYTES_WRITER(s32)
LONE_BYTES_WRITER(u64)
LONE_BYTES_WRITER(s64)

#undef LONE_BYTES_READER
#undef LONE_BYTES_WRITER

#define LONE_BYTES_CHECKED_READER(type) \
bool lone_bytes_checked_read_##type(struct lone_bytes bytes, lone_size offset, lone_##type *type) \
{ \
	if (!lone_bytes_contains_block(bytes, offset, sizeof(*type))) { \
		return false; \
	} else { \
		*type = lone_bytes_read_##type(bytes, offset); \
		return true; \
	} \
}

#define LONE_BYTES_CHECKED_WRITER(type) \
bool lone_bytes_checked_write_##type(struct lone_bytes bytes, lone_size offset, lone_##type type) \
{ \
	if (!lone_bytes_contains_block(bytes, offset, sizeof(type))) { \
		return false; \
	} else { \
		lone_bytes_write_##type(bytes, offset, type); \
		return true; \
	} \
}

LONE_BYTES_CHECKED_READER(u8)
LONE_BYTES_CHECKED_READER(s8)
LONE_BYTES_CHECKED_READER(u16)
LONE_BYTES_CHECKED_READER(s16)
LONE_BYTES_CHECKED_READER(u32)
LONE_BYTES_CHECKED_READER(s32)
LONE_BYTES_CHECKED_READER(u64)
LONE_BYTES_CHECKED_READER(s64)

LONE_BYTES_CHECKED_WRITER(u8)
LONE_BYTES_CHECKED_WRITER(s8)
LONE_BYTES_CHECKED_WRITER(u16)
LONE_BYTES_CHECKED_WRITER(s16)
LONE_BYTES_CHECKED_WRITER(u32)
LONE_BYTES_CHECKED_WRITER(s32)
LONE_BYTES_CHECKED_WRITER(u64)
LONE_BYTES_CHECKED_WRITER(s64)

#undef LONE_BYTES_CHECKED_READER
#undef LONE_BYTES_CHECKED_WRITER
