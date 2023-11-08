#include <lone/types.h>

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
