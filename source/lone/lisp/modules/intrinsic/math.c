/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/modules/intrinsic/math.h>
#include <lone/lisp/modules/intrinsic/lone.h>

#include <lone/lisp/machine.h>
#include <lone/lisp/machine/stack.h>
#include <lone/lisp/module.h>
#include <lone/lisp/utilities.h>

#include <lone/linux.h>

void lone_lisp_modules_intrinsic_math_initialize(struct lone_lisp *lone)
{
	struct lone_lisp_value name, module;
	struct lone_lisp_function_flags flags;

	name = lone_lisp_intern_c_string(lone, "math");
	module = lone_lisp_module_for_name(lone, name);
	flags.evaluate_arguments = true;
	flags.evaluate_result = false;

	lone_lisp_module_export_primitive(lone, module, "+",
			"add", lone_lisp_primitive_math_add, module, flags);

	lone_lisp_module_export_primitive(lone, module, "-",
			"subtract", lone_lisp_primitive_math_subtract, module, flags);

	lone_lisp_module_export_primitive(lone, module, "*",
			"multiply", lone_lisp_primitive_math_multiply, module, flags);

	lone_lisp_module_export_primitive(lone, module, "/",
			"divide", lone_lisp_primitive_math_divide, module, flags);

	lone_lisp_module_export_primitive(lone, module, "<",
			"is_less_than", lone_lisp_primitive_math_is_less_than, module, flags);

	lone_lisp_module_export_primitive(lone, module, "<=",
			"is_less_than_or_equal_to", lone_lisp_primitive_math_is_less_than_or_equal_to, module, flags);

	lone_lisp_module_export_primitive(lone, module, ">",
			"is_greater_than", lone_lisp_primitive_math_is_greater_than, module, flags);

	lone_lisp_module_export_primitive(lone, module, ">=",
			"is_greater_than_or_equal_to", lone_lisp_primitive_math_is_greater_than_or_equal_to, module, flags);

	lone_lisp_module_export_primitive(lone, module, "sign",
			"sign", lone_lisp_primitive_math_sign, module, flags);

	lone_lisp_module_export_primitive(lone, module, "zero?",
			"is_zero", lone_lisp_primitive_math_is_zero, module, flags);

	lone_lisp_module_export_primitive(lone, module, "positive?",
			"is_positive", lone_lisp_primitive_math_is_positive, module, flags);

	lone_lisp_module_export_primitive(lone, module, "negative?",
			"is_negative", lone_lisp_primitive_math_is_negative, module, flags);
}

static struct lone_lisp_optional_value lone_lisp_primitive_integer_operation(struct lone_lisp *lone, struct lone_lisp_value arguments, char operation, struct lone_lisp_value accumulator)
{
	struct lone_lisp_value argument;
	lone_lisp_integer x, y, result;

	if (lone_lisp_is_nil(arguments)) {
		/* wasn't given any arguments to operate on: (+), (-), (*) */
		return (struct lone_lisp_optional_value) { .present = true, .value = accumulator };
	}

	do {
		argument = lone_lisp_list_first(lone, arguments);

		switch (lone_lisp_type_of(argument)) {
		case LONE_LISP_TAG_INTEGER:
			switch (lone_lisp_type_of(accumulator)) {
			case LONE_LISP_TAG_INTEGER:
				x = lone_lisp_integer_of(accumulator);
				y = lone_lisp_integer_of(argument);

				switch (operation) {
				case '+':
					if (__builtin_add_overflow(x, y, &result)) { goto overflow; }
					break;
				case '-':
					if (__builtin_sub_overflow(x, y, &result)) { goto overflow; }
					break;
				case '*':
					if (__builtin_mul_overflow(x, y, &result)) { goto overflow; }
					break;
				default:
					/* invalid primitive integer operation */ linux_exit(-1);
				}

				if (result < LONE_LISP_INTEGER_MIN || result > LONE_LISP_INTEGER_MAX) {
					goto overflow;
				}

				accumulator = lone_lisp_integer_create(result);
				break;
			default:
				/* accumulator is not a number */ linux_exit(-1);
			}
			break;
		default:
			goto not_a_number;
		}

		arguments = lone_lisp_list_rest(lone, arguments);

	} while (!lone_lisp_is_nil(arguments));

	return (struct lone_lisp_optional_value) { .present = true, .value = accumulator };

overflow:
not_a_number:
	return (struct lone_lisp_optional_value) { .present = false, .value = argument };
}

static struct lone_lisp_value error_tag_for(struct lone_lisp *lone, struct lone_lisp_value value)
{
	char *tag = lone_lisp_is_integer(lone, value)? "integer-overflow" : "type-error";
	return lone_lisp_intern_c_string(lone, tag);
}

/* Combine one integer into the accumulator.
 * Caller must ensure accumulator is itself an integer.
 * On failure, returns the offending argument as the signal value:
 * either because argument was not an integer (type-error tag)
 * or because the operation overflowed (integer-overflow tag). */
static struct lone_lisp_optional_value combine_integers(struct lone_lisp *lone,
		struct lone_lisp_value accumulator, struct lone_lisp_value argument, char operation)
{
	lone_lisp_integer x, y, result;

	if (lone_lisp_type_of(argument) != LONE_LISP_TAG_INTEGER) { goto offending; }
	if (lone_lisp_type_of(accumulator) != LONE_LISP_TAG_INTEGER) { linux_exit(-1); }

	x = lone_lisp_integer_of(accumulator);
	y = lone_lisp_integer_of(argument);

	switch (operation) {
	case '+':
		if (__builtin_add_overflow(x, y, &result)) { goto offending; }
		break;
	case '-':
		if (__builtin_sub_overflow(x, y, &result)) { goto offending; }
		break;
	case '*':
		if (__builtin_mul_overflow(x, y, &result)) { goto offending; }
		break;
	default:
		/* invalid primitive integer operation */ linux_exit(-1);
	}

	if (result < LONE_LISP_INTEGER_MIN || result > LONE_LISP_INTEGER_MAX) { goto offending; }

	return (struct lone_lisp_optional_value) { .present = true, .value = lone_lisp_integer_create(result) };

offending:
	return (struct lone_lisp_optional_value) { .present = false, .value = argument };
}

LONE_LISP_PRIMITIVE(math_add)
{
	struct lone_lisp_value arguments, argument, accumulator;
	struct lone_lisp_optional_value result;

	switch (step) {
	case 0:

		arguments = lone_lisp_machine_pop_value(lone, machine);
		accumulator = lone_lisp_zero();

		goto iterate;

	case 1: /* resumed with a replacement argument */

		arguments   = lone_lisp_machine_pop_value(lone, machine);
		accumulator = lone_lisp_machine_pop_value(lone, machine);
		argument    = machine->value;

		goto combine;

	default:
		break;
	}

	linux_exit(-1);

iterate:

	if (lone_lisp_is_nil(arguments)) {
		lone_lisp_machine_push_value(lone, machine, accumulator);
		return 0;
	}

	argument = lone_lisp_list_first(lone, arguments);

combine:

	result = combine_integers(lone, accumulator, argument, '+');

	if (!result.present) {
		lone_lisp_machine_push_value(lone, machine, accumulator);
		lone_lisp_machine_push_value(lone, machine, arguments);
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				1,
				error_tag_for(lone, result.value),
				result.value
			);
	}

	accumulator = result.value;
	arguments = lone_lisp_list_rest(lone, arguments);

	goto iterate;
}

LONE_LISP_PRIMITIVE(math_subtract)
{
	struct lone_lisp_value arguments, argument, accumulator;
	struct lone_lisp_optional_value result;

	switch (step) {
	case 0:

		arguments = lone_lisp_machine_pop_value(lone, machine);

		if (!lone_lisp_is_nil(arguments) && !lone_lisp_is_nil(lone_lisp_list_rest(lone, arguments))) {
			/* at least two arguments, set initial value to the first argument: (- 100 58) */
			argument  = lone_lisp_list_first(lone, arguments);
			arguments = lone_lisp_list_rest(lone, arguments);

			goto validate_first;
		}

		accumulator = lone_lisp_zero();

		goto iterate;

	case 1: /* resumed with a replacement for the first argument */

		arguments = lone_lisp_machine_pop_value(lone, machine);
		argument  = machine->value;

		goto validate_first;

	case 2: /* resumed with a replacement argument mid-iteration */

		arguments   = lone_lisp_machine_pop_value(lone, machine);
		accumulator = lone_lisp_machine_pop_value(lone, machine);
		argument    = machine->value;

		goto combine;

	default:
		break;
	}

	linux_exit(-1);

validate_first:

	if (!lone_lisp_is_integer(lone, argument)) {
		lone_lisp_machine_push_value(lone, machine, arguments);
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				1,
				lone_lisp_intern_c_string(lone, "type-error"),
				argument
			);
	}

	accumulator = argument;

iterate:

	if (lone_lisp_is_nil(arguments)) {
		lone_lisp_machine_push_value(lone, machine, accumulator);
		return 0;
	}

	argument = lone_lisp_list_first(lone, arguments);

combine:

	result = combine_integers(lone, accumulator, argument, '-');

	if (!result.present) {
		lone_lisp_machine_push_value(lone, machine, accumulator);
		lone_lisp_machine_push_value(lone, machine, arguments);
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				2,
				error_tag_for(lone, result.value),
				result.value
			);
	}

	accumulator = result.value;
	arguments = lone_lisp_list_rest(lone, arguments);

	goto iterate;
}

LONE_LISP_PRIMITIVE(math_multiply)
{
	struct lone_lisp_value arguments, argument, accumulator;
	struct lone_lisp_optional_value result;

	switch (step) {
	case 0:

		arguments = lone_lisp_machine_pop_value(lone, machine);
		accumulator = lone_lisp_one();

		goto iterate;

	case 1: /* resumed with a replacement argument */

		arguments   = lone_lisp_machine_pop_value(lone, machine);
		accumulator = lone_lisp_machine_pop_value(lone, machine);
		argument    = machine->value;

		goto combine;

	default:
		break;
	}

	linux_exit(-1);

iterate:

	if (lone_lisp_is_nil(arguments)) {
		lone_lisp_machine_push_value(lone, machine, accumulator);
		return 0;
	}

	argument = lone_lisp_list_first(lone, arguments);

combine:

	result = combine_integers(lone, accumulator, argument, '*');

	if (!result.present) {
		lone_lisp_machine_push_value(lone, machine, accumulator);
		lone_lisp_machine_push_value(lone, machine, arguments);
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				1,
				error_tag_for(lone, result.value),
				result.value
			);
	}

	accumulator = result.value;
	arguments = lone_lisp_list_rest(lone, arguments);

	goto iterate;
}

LONE_LISP_PRIMITIVE(math_divide)
{
	struct lone_lisp_value arguments, dividend, divisor, result;
	struct lone_lisp_optional_value divisor_result;
	lone_lisp_integer x, y, quotient;

	switch (step) {
	case 0:

		arguments = lone_lisp_machine_pop_value(lone, machine);

		if (lone_lisp_is_nil(arguments)) { /* at least the dividend is required, (/) is invalid */ goto no_arguments; }
		dividend = lone_lisp_list_first(lone, arguments);
		arguments = lone_lisp_list_rest(lone, arguments);

		goto validate_dividend;

	case 1: /* resumed with a replacement divisor */

		dividend = lone_lisp_machine_pop_value(lone, machine);
		divisor  = machine->value;

		goto validate_divisor;

	case 2: /* resumed with a replacement dividend */

		arguments = lone_lisp_machine_pop_value(lone, machine);
		dividend  = machine->value;

		goto validate_dividend;

	default:
		break;
	}

	linux_exit(-1);

validate_dividend:

	switch (lone_lisp_type_of(dividend)) {
	case LONE_LISP_TAG_INTEGER:
		if (lone_lisp_is_nil(arguments)) {
			/* not given a divisor, return 1/x instead: (/ 2) = 1/2 */
			divisor = dividend;
			dividend = lone_lisp_one();
			break;
		} else {
			/* (/ x a b c ...) = x / (a * b * c * ...) */
			divisor_result = lone_lisp_primitive_integer_operation(lone, arguments, '*', lone_lisp_one());
			if (!divisor_result.present) {
				lone_lisp_machine_push_value(lone, machine, dividend);
				return
					lone_lisp_signal_emit(
						lone,
						machine,
						1,
						error_tag_for(lone, divisor_result.value),
						divisor_result.value
					);
			}
			divisor = divisor_result.value;
			break;
		}
	default:
		lone_lisp_machine_push_value(lone, machine, arguments);
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				2,
				lone_lisp_intern_c_string(lone, "type-error"),
				dividend
			);
	}

validate_divisor:

	if (!lone_lisp_is_integer(lone, divisor)) {
		lone_lisp_machine_push_value(lone, machine, dividend);
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				1,
				lone_lisp_intern_c_string(lone, "type-error"),
				divisor
			);
	}

	if (lone_lisp_integer_of(divisor) == 0) {
		lone_lisp_machine_push_value(lone, machine, dividend);
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				1,
				lone_lisp_intern_c_string(lone, "division-by-zero"),
				dividend
			);
	}

	x = lone_lisp_integer_of(dividend);
	y = lone_lisp_integer_of(divisor);

	quotient = x / y;

	if (quotient < LONE_LISP_INTEGER_MIN || quotient > LONE_LISP_INTEGER_MAX) {
		lone_lisp_machine_push_value(lone, machine, dividend);
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				1,
				lone_lisp_intern_c_string(lone, "integer-overflow"),
				divisor
			);
	}

	result = lone_lisp_integer_create(quotient);

	lone_lisp_machine_push_value(lone, machine, result);
	return 0;

no_arguments:
		linux_exit(-1);
}

static long apply_and_return(struct lone_lisp *lone, struct lone_lisp_machine *machine,
		lone_lisp_comparator_function comparator)
{
	struct lone_lisp_value arguments, result;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	result = lone_lisp_apply_comparator(lone, arguments, comparator);

	lone_lisp_machine_push_value(lone, machine, result);
	return 0;
}

LONE_LISP_PRIMITIVE(math_is_less_than)
{
	return apply_and_return(lone, machine, lone_lisp_integer_is_less_than);
}

LONE_LISP_PRIMITIVE(math_is_less_than_or_equal_to)
{
	return apply_and_return(lone, machine, lone_lisp_integer_is_less_than_or_equal_to);
}

LONE_LISP_PRIMITIVE(math_is_greater_than)
{
	return apply_and_return(lone, machine, lone_lisp_integer_is_greater_than);
}

LONE_LISP_PRIMITIVE(math_is_greater_than_or_equal_to)
{
	return apply_and_return(lone, machine, lone_lisp_integer_is_greater_than_or_equal_to);
}

static struct lone_lisp_value sign_of(struct lone_lisp_value value)
{
	lone_lisp_integer x = lone_lisp_integer_of(value);

	if (x > 0) {
		return lone_lisp_one();
	} else if (x < 0) {
		return lone_lisp_minus_one();
	} else {
		return lone_lisp_zero();
	}
}

static struct lone_lisp_value compute_sign(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	struct lone_lisp_value arguments, value;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	if (lone_lisp_list_destructure(lone, arguments, 1, &value)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	switch (lone_lisp_type_of(value)) {
	case LONE_LISP_TAG_INTEGER:
		return sign_of(value);

	default:
		/* value is not a number */ linux_exit(-1);
	}
}

LONE_LISP_PRIMITIVE(math_sign)
{
	lone_lisp_machine_push_value(lone, machine, compute_sign(lone, machine));
	return 0;
}

static long sign_predicate(struct lone_lisp *lone, struct lone_lisp_machine *machine, struct lone_lisp_value sign)
{
	lone_lisp_integer x, y;

	x = lone_lisp_integer_of(compute_sign(lone, machine));
	y = lone_lisp_integer_of(sign);

	lone_lisp_machine_push_value(lone, machine, lone_lisp_boolean_for(x == y));
	return 0;
}

LONE_LISP_PRIMITIVE(math_is_zero)
{
	return sign_predicate(lone, machine, lone_lisp_zero());
}

LONE_LISP_PRIMITIVE(math_is_positive)
{
	return sign_predicate(lone, machine, lone_lisp_one());
}

LONE_LISP_PRIMITIVE(math_is_negative)
{
	return sign_predicate(lone, machine, lone_lisp_minus_one());
}
