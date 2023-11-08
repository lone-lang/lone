#include <lone/constants.h>

#include <lone/struct/lisp.h>

struct lone_value *lone_nil(struct lone_lisp *lone)
{
	return lone->constants.nil;
}

struct lone_value *lone_true(struct lone_lisp *lone)
{
	return lone->constants.truth;
}
