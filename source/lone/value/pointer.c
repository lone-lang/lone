#include <lone/value.h>
#include <lone/value/pointer.h>

#include <lone/struct/value.h>
#include <lone/struct/pointer.h>

struct lone_value *lone_pointer_create(struct lone_lisp *lone, void *pointer, enum lone_pointer_type type)
{
	struct lone_value *value = lone_value_create(lone);
	value->type = LONE_POINTER;
	value->pointer.type = type;
	value->pointer.address = pointer;
	return value;
}
