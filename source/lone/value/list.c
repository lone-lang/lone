#include <lone/value.h>
#include <lone/value/list.h>

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
