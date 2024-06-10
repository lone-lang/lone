/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/modules/intrinsic/math.h>
#include <lone/modules.h>

#include <lone/value/primitive.h>
#include <lone/value/list.h>
#include <lone/value/table.h>
#include <lone/value/symbol.h>
#include <lone/value/integer.h>

#include <lone/utilities.h>
#include <lone/linux.h>

void lone_modules_intrinsic_math_initialize(struct lone_lisp *lone)
{
	struct lone_value name, module, primitive;
	struct lone_function_flags flags;

	name = lone_intern_c_string(lone, "math");
	module = lone_module_for_name(lone, name);
	flags.evaluate_arguments = true;
	flags.evaluate_result = false;

	primitive = lone_primitive_create(lone, "add", lone_primitive_math_add, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "+"), primitive);

	primitive = lone_primitive_create(lone, "subtract", lone_primitive_math_subtract, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "-"), primitive);

	primitive = lone_primitive_create(lone, "multiply", lone_primitive_math_multiply, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "*"), primitive);

	primitive = lone_primitive_create(lone, "divide", lone_primitive_math_divide, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "/"), primitive);

	primitive = lone_primitive_create(lone, "is_less_than", lone_primitive_math_is_less_than, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "<"), primitive);

	primitive = lone_primitive_create(lone, "is_less_than_or_equal_to", lone_primitive_math_is_less_than_or_equal_to, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "<="), primitive);

	primitive = lone_primitive_create(lone, "is_greater_than", lone_primitive_math_is_greater_than, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, ">"), primitive);

	primitive = lone_primitive_create(lone, "is_greater_than_or_equal_to", lone_primitive_math_is_greater_than_or_equal_to, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, ">="), primitive);

	primitive = lone_primitive_create(lone, "sign", lone_primitive_math_sign, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "sign"), primitive);

	primitive = lone_primitive_create(lone, "is_zero", lone_primitive_math_is_zero, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "zero?"), primitive);

	primitive = lone_primitive_create(lone, "is_positive", lone_primitive_math_is_positive, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "positive?"), primitive);

	primitive = lone_primitive_create(lone, "is_negative", lone_primitive_math_is_negative, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "negative?"), primitive);

	lone_table_set(lone, lone->modules.loaded, name, module);
}

static struct lone_value lone_primitive_integer_operation(struct lone_lisp *lone, struct lone_value arguments, char operation, struct lone_value accumulator)
{
	struct lone_value argument;

	if (lone_is_nil(arguments)) {
		/* wasn't given any arguments to operate on: (+), (-), (*) */
		return accumulator;
	}

	do {
		argument = lone_list_first(arguments);

		switch (argument.type) {
		case LONE_TYPE_INTEGER:
			switch (accumulator.type) {
			case LONE_TYPE_INTEGER:
				switch (operation) {
				case '+':
					accumulator.as.signed_integer += argument.as.signed_integer;
					break;
				case '-':
					accumulator.as.signed_integer -= argument.as.signed_integer;
					break;
				case '*':
					accumulator.as.signed_integer *= argument.as.signed_integer;
					break;
				default:
					/* invalid primitive integer operation */ linux_exit(-1);
				}
				break;
			case LONE_TYPE_NIL:
			case LONE_TYPE_HEAP_VALUE:
			case LONE_TYPE_POINTER:
				/* accumulator is not a number */ linux_exit(-1);
			}
			break;
		case LONE_TYPE_NIL:
		case LONE_TYPE_HEAP_VALUE:
		case LONE_TYPE_POINTER:
			/* argument is not a number */ linux_exit(-1);
		}

		arguments = lone_list_rest(arguments);

	} while (!lone_is_nil(arguments));

	return accumulator;
}

LONE_PRIMITIVE(math_add)
{
	return lone_primitive_integer_operation(lone, arguments, '+', lone_zero());
}

LONE_PRIMITIVE(math_subtract)
{
	struct lone_value first, accumulator;

	if (!lone_is_nil(arguments) && !lone_is_nil(lone_list_rest(arguments))) {
		/* at least two arguments, set initial value to the first argument: (- 100 58) */
		first = lone_list_first(arguments);
		if (!lone_is_integer(first)) { /* argument is not a number */ linux_exit(-1); }
		accumulator = first;
		arguments = lone_list_rest(arguments);
	} else {
		accumulator = lone_zero();
	}

	return lone_primitive_integer_operation(lone, arguments, '-', accumulator);
}

LONE_PRIMITIVE(math_multiply)
{
	return lone_primitive_integer_operation(lone, arguments, '*', lone_one());
}

LONE_PRIMITIVE(math_divide)
{
	struct lone_value dividend, divisor;

	if (lone_is_nil(arguments)) { /* at least the dividend is required, (/) is invalid */ linux_exit(-1); }
	dividend = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);

	switch (dividend.type) {
	case LONE_TYPE_INTEGER:
		if (lone_is_nil(arguments)) {
			/* not given a divisor, return 1/x instead: (/ 2) = 1/2 */
			return lone_integer_create(1 / dividend.as.signed_integer);
		} else {
			/* (/ x a b c ...) = x / (a * b * c * ...) */
			divisor = lone_primitive_integer_operation(lone, arguments, '*', lone_one());
			return lone_integer_create(dividend.as.signed_integer / divisor.as.signed_integer);
		}
	case LONE_TYPE_NIL:
	case LONE_TYPE_POINTER:
	case LONE_TYPE_HEAP_VALUE:
		/* can't divide non-numbers: (/ "not a number") */ linux_exit(-1);
	}
}

LONE_PRIMITIVE(math_is_less_than)
{
	return lone_apply_comparator(lone, arguments, lone_integer_is_less_than);
}

LONE_PRIMITIVE(math_is_less_than_or_equal_to)
{
	return lone_apply_comparator(lone, arguments, lone_integer_is_less_than_or_equal_to);
}

LONE_PRIMITIVE(math_is_greater_than)
{
	return lone_apply_comparator(lone, arguments, lone_integer_is_greater_than);
}

LONE_PRIMITIVE(math_is_greater_than_or_equal_to)
{
	return lone_apply_comparator(lone, arguments, lone_integer_is_greater_than_or_equal_to);
}

LONE_PRIMITIVE(math_sign)
{
	struct lone_value value;

	if (lone_list_destructure(arguments, 1, &value)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	switch (value.type) {
	case LONE_TYPE_INTEGER:
		if (value.as.signed_integer > 0) { return lone_one(); }
		else if (value.as.signed_integer < 0) { return lone_minus_one(); }
		else { return lone_zero(); }
	case LONE_TYPE_NIL:
	case LONE_TYPE_POINTER:
	case LONE_TYPE_HEAP_VALUE:
		/* value is not a number */ linux_exit(-1);
	}
}

LONE_PRIMITIVE(math_is_zero)
{
	struct lone_value value = lone_primitive_math_sign(lone, module, environment, arguments, closure);
	if (value.as.signed_integer == 0) { return value; }
	else { return lone_nil(); }
}

LONE_PRIMITIVE(math_is_positive)
{
	struct lone_value value = lone_primitive_math_sign(lone, module, environment, arguments, closure);
	if (value.as.signed_integer > 0) { return value; }
	else { return lone_nil(); }
}

LONE_PRIMITIVE(math_is_negative)
{
	struct lone_value value = lone_primitive_math_sign(lone, module, environment, arguments, closure);
	if (value.as.signed_integer < 0) { return value; }
	else { return lone_nil(); }
}
