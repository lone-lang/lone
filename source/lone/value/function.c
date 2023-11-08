#include <lone/value.h>
#include <lone/value/function.h>

#include <lone/struct/value.h>
#include <lone/struct/function.h>

struct lone_value *lone_function_create(
	struct lone_lisp *lone,
	struct lone_value *arguments,
	struct lone_value *code,
	struct lone_value *environment,
	struct lone_function_flags flags)
{
	struct lone_value *value = lone_value_create(lone);
	value->type = LONE_FUNCTION;
	value->function.arguments = arguments;
	value->function.code = code;
	value->function.environment = environment;
	value->function.flags = flags;
	return value;
}
