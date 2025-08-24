#include <lone/lisp/machine.h>

#include <lone/stack.h>
#include <lone/linux.h>

static bool lone_lisp_machine_can_push(struct lone_lisp_machine *machine)
{
	return lone_stack_can_push(machine->stack.top, machine->stack.limit, sizeof(struct lone_lisp_machine_stack_frame));
}

static bool lone_lisp_machine_can_pop(struct lone_lisp_machine *machine)
{
	return lone_stack_can_pop(machine->stack.top, machine->stack.base, sizeof(struct lone_lisp_machine_stack_frame));
}

void lone_lisp_machine_push(struct lone_lisp *lone, struct lone_lisp_machine *machine,
		struct lone_lisp_machine_stack_frame frame)
{
	if (!lone_lisp_machine_can_push(machine)) { linux_exit(-1); }
	*machine->stack.top++ = frame;
}

struct lone_lisp_machine_stack_frame lone_lisp_machine_pop(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	if (!lone_lisp_machine_can_pop(machine)) { linux_exit(-1); }
	return *--machine->stack.top;
}

void lone_lisp_machine_push_value(struct lone_lisp *lone, struct lone_lisp_machine *machine,
		struct lone_lisp_value value)
{
	lone_lisp_machine_push(lone, machine, (struct lone_lisp_machine_stack_frame) {
		.as.value = value,
	});
}

struct lone_lisp_value lone_lisp_machine_pop_value(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	struct lone_lisp_value value = lone_lisp_machine_pop(lone, machine).as.value;
	return value;
}

void lone_lisp_machine_push_integer(struct lone_lisp *lone, struct lone_lisp_machine *machine,
		lone_lisp_integer integer)
{
	lone_lisp_machine_push(lone, machine, (struct lone_lisp_machine_stack_frame) {
		.as.integer = integer,
	});
}

lone_lisp_integer lone_lisp_machine_pop_integer(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	return lone_lisp_machine_pop(lone, machine).as.integer;
}

void lone_lisp_machine_push_step(struct lone_lisp *lone, struct lone_lisp_machine *machine,
		enum lone_lisp_machine_step step)
{
	lone_lisp_machine_push(lone, machine, (struct lone_lisp_machine_stack_frame) {
		.as.step = step,
	});
}

void lone_lisp_machine_save_step(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	lone_lisp_machine_push_step(lone, machine, machine->step);
}

enum lone_lisp_machine_step lone_lisp_machine_pop_step(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	enum lone_lisp_machine_step step = lone_lisp_machine_pop(lone, machine).as.step;
	return step;
}

void lone_lisp_machine_restore_step(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	machine->step = lone_lisp_machine_pop_step(lone, machine);
}

void lone_lisp_machine_push_primitive_step(struct lone_lisp *lone, struct lone_lisp_machine *machine,
		long primitive_step)
{
	lone_lisp_machine_push(lone, machine, (struct lone_lisp_machine_stack_frame) {
		.as.primitive_step = primitive_step,
	});
}

void lone_lisp_machine_save_primitive_step(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	lone_lisp_machine_push_primitive_step(lone, machine, machine->primitive.step);
}

long lone_lisp_machine_pop_primitive_step(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	long primitive_step = lone_lisp_machine_pop(lone, machine).as.primitive_step;
	return primitive_step;
}

void lone_lisp_machine_restore_primitive_step(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	machine->primitive.step = lone_lisp_machine_pop_primitive_step(lone, machine);
}

static bool should_evaluate_operands(struct lone_lisp_value applicable, struct lone_lisp_value operands)
{
	if (lone_lisp_is_nil(operands)) {
		return false;
	} else {
		switch (lone_lisp_heap_value_of(applicable)->type) {
		case LONE_LISP_TYPE_FUNCTION:
			return lone_lisp_heap_value_of(applicable)->as.function.flags.evaluate_arguments;
		case LONE_LISP_TYPE_PRIMITIVE:
			return lone_lisp_heap_value_of(applicable)->as.primitive.flags.evaluate_arguments;
		case LONE_LISP_TYPE_VECTOR:
		case LONE_LISP_TYPE_TABLE:
			return true;
		default:
			linux_exit(-1);
		}
	}
}

static struct lone_lisp_value apply_to_collection(struct lone_lisp *lone,
		struct lone_lisp_value collection, struct lone_lisp_value arguments,
		struct lone_lisp_value (*get)(struct lone_lisp *, struct lone_lisp_value, struct lone_lisp_value),
		void (*set)(struct lone_lisp *, struct lone_lisp_value, struct lone_lisp_value, struct lone_lisp_value))
{
	struct lone_lisp_value key, value;

	if (lone_lisp_is_nil(arguments)) { /* need at least the key: (collection) */ linux_exit(-1); }

	key = lone_lisp_list_first(arguments);
	arguments = lone_lisp_list_rest(arguments);

	if (lone_lisp_is_nil(arguments)) {
		return get(lone, collection, key);
	} else {
		/* at least one argument */
		value = lone_lisp_list_first(arguments);
		arguments = lone_lisp_list_rest(arguments);

		if (lone_lisp_is_nil(arguments)) {
			/* collection set: (collection key value) */
			set(lone, collection, key, value);
			return value;
		} else {
			/* too many arguments given: (collection key value extra) */
			linux_exit(-1);
		}
	}
}

static struct lone_lisp_value apply_to_vector(struct lone_lisp *lone,
		struct lone_lisp_value vector, struct lone_lisp_value arguments)
{
	return apply_to_collection(lone, vector, arguments, lone_lisp_vector_get, lone_lisp_vector_set);
}

static struct lone_lisp_value apply_to_table(struct lone_lisp *lone,
		struct lone_lisp_value table, struct lone_lisp_value arguments)
{
	return apply_to_collection(lone, table, arguments, lone_lisp_table_get, lone_lisp_table_set);
}

static struct lone_lisp_value bind_arguments(struct lone_lisp *lone, struct lone_lisp_value environment,
		struct lone_lisp_value function, struct lone_lisp_value arguments)
{
	struct lone_lisp_value new_environment, names, current;

	names = lone_lisp_heap_value_of(function)->as.function.arguments;

	new_environment = lone_lisp_table_create(
		lone,
		16,
		lone_lisp_heap_value_of(function)->as.function.environment
	);

	while (1) {
		if (!lone_lisp_is_nil(names)) {
			current = lone_lisp_list_first(names);

			switch (lone_lisp_type_of(current)) {
			case LONE_LISP_TYPE_HEAP_VALUE:
				break;
			case LONE_LISP_TYPE_NIL:
			case LONE_LISP_TYPE_FALSE:
			case LONE_LISP_TYPE_TRUE:
			case LONE_LISP_TYPE_INTEGER:
				/* unexpected value */ linux_exit(-1);
			}

			switch (lone_lisp_heap_value_of(current)->type) {
			case LONE_LISP_TYPE_SYMBOL:
				/* normal argument passing: (lambda (x y)) */

				if (!lone_lisp_is_nil(arguments)) {
					/* argument matched to name, set name in environment */
					lone_lisp_table_set(
						lone,
						new_environment,
						current,
						lone_lisp_list_first(arguments)
					);
				} else {
					/* argument number mismatch: ((lambda (x y) y) 10) */ linux_exit(-1);
				}

				break;
			case LONE_LISP_TYPE_LIST:
				/* variadic argument passing: (lambda ((arguments))), (lambda (x y (rest))) */

				if (!lone_lisp_is_symbol(lone_lisp_list_first(current))) {
					/* no name given: (lambda (x y ())) */ linux_exit(-1);
				} else if (lone_lisp_list_has_rest(current)) {
					/* too many names given: (lambda (x y (rest extra))) */ linux_exit(-1);
				} else {
					/* match list of remaining arguments to name */
					lone_lisp_table_set(
						lone,
						new_environment,
						lone_lisp_list_first(current),
						arguments
					);

					return new_environment;
				}

			case LONE_LISP_TYPE_MODULE:
			case LONE_LISP_TYPE_FUNCTION:
			case LONE_LISP_TYPE_PRIMITIVE:
			case LONE_LISP_TYPE_BYTES:
			case LONE_LISP_TYPE_TEXT:
			case LONE_LISP_TYPE_VECTOR:
			case LONE_LISP_TYPE_TABLE:
				/* unexpected value */ linux_exit(-1);
			}

			names = lone_lisp_list_rest(names);
			arguments = lone_lisp_list_rest(arguments);

		} else if (!lone_lisp_is_nil(arguments)) {
			/* argument number mismatch: ((lambda (x) x) 10 20) */ linux_exit(-1);
		} else {
			/* matching number of arguments */
			break;
		}
	}

	return new_environment;
}

void lone_lisp_machine_reset(struct lone_lisp *lone,
		struct lone_lisp_value module, struct lone_lisp_value expression)
{
	struct lone_lisp_machine *machine;

	machine = &lone->machine;

	machine->stack.top = machine->stack.base;

	machine->step = LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION;
	lone_lisp_machine_push_step(lone, machine, LONE_LISP_MACHINE_STEP_HALT);
	machine->primitive.step = 0;

	machine->expression = expression;
	machine->module = module;
	machine->environment = lone_lisp_heap_value_of(module)->as.module.environment;
}
