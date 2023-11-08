#include <lone/value.h>
#include <lone/value/primitive.h>

#include <lone/struct/value.h>
#include <lone/struct/primitive.h>

struct lone_value *lone_intern_c_string(struct lone_lisp *, char *);

struct lone_value *lone_primitive_create(
	struct lone_lisp *lone,
	char *name,
	lone_primitive function,
	struct lone_value *closure,
	struct lone_function_flags flags)
{
	struct lone_value *value = lone_value_create(lone);
	value->type = LONE_PRIMITIVE;
	value->primitive.name = lone_intern_c_string(lone, name);
	value->primitive.function = function;
	value->primitive.closure = closure;
	value->primitive.flags = flags;
	value->primitive.flags.variable_arguments = 1;
	return value;
}

