#include <lone/lisp/types.h>
#include <lone/lisp/machine.h>

#include <lone/memory/functions.h>
#include <lone/memory/array.h>

#include <lone/stack.h>
#include <lone/linux.h>

static bool lone_lisp_machine_can_push_bytes(struct lone_lisp_machine *machine, size_t bytes)
{
	return lone_stack_can_push(machine->stack.top, machine->stack.limit, bytes);
}

static bool lone_lisp_machine_can_push(struct lone_lisp_machine *machine)
{
	return lone_lisp_machine_can_push_bytes(machine, sizeof(struct lone_lisp_machine_stack_frame));
}

static bool lone_lisp_machine_can_pop(struct lone_lisp_machine *machine)
{
	return lone_stack_can_pop(machine->stack.top, machine->stack.base, sizeof(struct lone_lisp_machine_stack_frame));
}

static bool lone_lisp_machine_can_peek(struct lone_lisp_machine *machine, size_t depth)
{
	return ((size_t) (machine->stack.top - machine->stack.base)) >= depth;
}

void lone_lisp_machine_push(struct lone_lisp *lone, struct lone_lisp_machine *machine,
		struct lone_lisp_machine_stack_frame frame)
{
	if (!lone_lisp_machine_can_push(machine)) { linux_exit(-1); }
	*machine->stack.top++ = frame;
}

void lone_lisp_machine_push_frames(struct lone_lisp *lone, struct lone_lisp_machine *machine,
		size_t frame_count, struct lone_lisp_machine_stack_frame *frames)
{
	size_t size = lone_memory_array_size_in_bytes(frame_count, sizeof(*frames));
	if (!lone_lisp_machine_can_push_bytes(machine, size)) { linux_exit(-1); }
	lone_memory_move(frames, machine->stack.top, size);
	machine->stack.top += frame_count;
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
		.tagged = value.tagged,
	});
}

void lone_lisp_machine_push_function_delimiter(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	lone_lisp_machine_push(lone, machine, (struct lone_lisp_machine_stack_frame) {
		.tagged = LONE_LISP_TAG_FUNCTION_DELIMITER,
	});
}

void lone_lisp_machine_push_continuation_delimiter(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	lone_lisp_machine_push(lone, machine, (struct lone_lisp_machine_stack_frame) {
		.tagged = LONE_LISP_TAG_CONTINUATION_DELIMITER,
	});
}

struct lone_lisp_value lone_lisp_machine_pop_value(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	return (struct lone_lisp_value) { .tagged = lone_lisp_machine_pop(lone, machine).tagged };
}

struct lone_lisp_value lone_lisp_machine_peek_value(struct lone_lisp *lone, struct lone_lisp_machine *machine,
		size_t depth)
{
	if (!lone_lisp_machine_can_peek(machine, depth)) { linux_exit(-1); }
	return (struct lone_lisp_value) { .tagged = (machine->stack.top - depth)->tagged };
}

void lone_lisp_machine_pop_function_delimiter(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	lone_lisp_machine_pop(lone, machine);
}

void lone_lisp_machine_pop_continuation_delimiter(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	lone_lisp_machine_pop(lone, machine);
}

void lone_lisp_machine_push_interceptor_delimiter(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	lone_lisp_machine_push(lone, machine, (struct lone_lisp_machine_stack_frame) {
		.tagged = LONE_LISP_TAG_INTERCEPTOR_DELIMITER,
	});
}

void lone_lisp_machine_pop_interceptor_delimiter(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	lone_lisp_machine_pop(lone, machine);
}

void lone_lisp_machine_push_integer(struct lone_lisp *lone, struct lone_lisp_machine *machine,
		lone_lisp_integer integer)
{
	lone_lisp_machine_push(lone, machine, (struct lone_lisp_machine_stack_frame) {
		.tagged = (integer << LONE_LISP_DATA_SHIFT) | LONE_LISP_TAG_INTEGER,
	});
}

lone_lisp_integer lone_lisp_machine_pop_integer(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	return lone_lisp_machine_pop(lone, machine).tagged >> LONE_LISP_DATA_SHIFT;
}

void lone_lisp_machine_push_step(struct lone_lisp *lone, struct lone_lisp_machine *machine,
		enum lone_lisp_machine_step step)
{
	lone_lisp_machine_push(lone, machine, (struct lone_lisp_machine_stack_frame) {
		.tagged = ((long) step << LONE_LISP_DATA_SHIFT) | LONE_LISP_TAG_STEP,
	});
}

void lone_lisp_machine_save_step(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	lone_lisp_machine_push_step(lone, machine, machine->step);
}

enum lone_lisp_machine_step lone_lisp_machine_pop_step(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	return lone_lisp_machine_pop(lone, machine).tagged >> LONE_LISP_DATA_SHIFT;
}

void lone_lisp_machine_restore_step(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	machine->step = lone_lisp_machine_pop_step(lone, machine);
}

void lone_lisp_machine_push_primitive_step(struct lone_lisp *lone, struct lone_lisp_machine *machine,
		long primitive_step)
{
	lone_lisp_machine_push(lone, machine, (struct lone_lisp_machine_stack_frame) {
		.tagged = (primitive_step << LONE_LISP_DATA_SHIFT) | LONE_LISP_TAG_PRIMITIVE_STEP,
	});
}

void lone_lisp_machine_save_primitive_step(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	lone_lisp_machine_push_primitive_step(lone, machine, machine->primitive.step);
}

long lone_lisp_machine_pop_primitive_step(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	return lone_lisp_machine_pop(lone, machine).tagged >> LONE_LISP_DATA_SHIFT;
}

void lone_lisp_machine_restore_primitive_step(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	machine->primitive.step = lone_lisp_machine_pop_primitive_step(lone, machine);
}

void lone_lisp_machine_unwind_to(struct lone_lisp *lone, struct lone_lisp_machine *machine, enum lone_lisp_tag tag)
{
	struct lone_lisp_machine_stack_frame frame;

	while (tag != ((enum lone_lisp_tag) ((frame = lone_lisp_machine_pop(lone, machine)).tagged & LONE_LISP_TAG_MASK)));

	lone_lisp_machine_push(lone, machine, frame);
}

void lone_lisp_machine_unwind_to_function_delimiter(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	lone_lisp_machine_unwind_to(lone, machine, LONE_LISP_TAG_FUNCTION_DELIMITER);
}

void lone_lisp_machine_unwind_to_interceptor_delimiter(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	lone_lisp_machine_unwind_to(lone, machine, LONE_LISP_TAG_INTERCEPTOR_DELIMITER);
}

static bool should_evaluate_operands(struct lone_lisp *lone,
		struct lone_lisp_value applicable, struct lone_lisp_value operands)
{
	if (lone_lisp_is_nil(operands)) {
		return false;
	} else {
		switch (applicable.tagged & LONE_LISP_TAG_MASK) {
		case LONE_LISP_TAG_FUNCTION:
		case LONE_LISP_TAG_PRIMITIVE:
			/* FEXPR flags are encoded in the metadata field (bit 8).
			 * Single bit test on the tagged word, no heap dereference.
			 */
			return (applicable.tagged & LONE_LISP_METADATA_EVALUATE_ARGUMENTS) != 0;
		case LONE_LISP_TAG_GENERATOR:
			return should_evaluate_operands(
				lone,
				lone_lisp_heap_value_of(lone, applicable)->as.generator.function,
				operands
			);
		case LONE_LISP_TAG_CONTINUATION:
		case LONE_LISP_TAG_VECTOR:
		case LONE_LISP_TAG_TABLE:
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

	key = lone_lisp_list_first(lone, arguments);
	arguments = lone_lisp_list_rest(lone, arguments);

	if (lone_lisp_is_nil(arguments)) {
		return get(lone, collection, key);
	} else {
		/* at least one argument */
		value = lone_lisp_list_first(lone, arguments);
		arguments = lone_lisp_list_rest(lone, arguments);

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

	names = lone_lisp_heap_value_of(lone, function)->as.function.arguments;

	new_environment = lone_lisp_table_create(
		lone,
		16,
		lone_lisp_heap_value_of(lone, function)->as.function.environment
	);

	while (1) {
		if (!lone_lisp_is_nil(names)) {
			current = lone_lisp_list_first(lone, names);

			switch (lone_lisp_type_of(current)) {
			case LONE_LISP_TAG_SYMBOL:
				/* normal argument passing: (lambda (x y)) */

				if (!lone_lisp_is_nil(arguments)) {
					/* argument matched to name, set name in environment */
					lone_lisp_table_set(
						lone,
						new_environment,
						current,
						lone_lisp_list_first(lone, arguments)
					);
				} else {
					/* argument number mismatch: ((lambda (x y) y) 10) */ linux_exit(-1);
				}

				break;
			case LONE_LISP_TAG_LIST:
				/* variadic argument passing: (lambda ((arguments))), (lambda (x y (rest))) */

				if (!lone_lisp_is_symbol(lone, lone_lisp_list_first(lone, current))) {
					/* no name given: (lambda (x y ())) */ linux_exit(-1);
				} else if (lone_lisp_list_has_rest(lone, current)) {
					/* too many names given: (lambda (x y (rest extra))) */ linux_exit(-1);
				} else {
					/* match list of remaining arguments to name */
					lone_lisp_table_set(
						lone,
						new_environment,
						lone_lisp_list_first(lone, current),
						arguments
					);

					return new_environment;
				}

			default:
				/* unexpected value */ linux_exit(-1);
			}

			names = lone_lisp_list_rest(lone, names);
			arguments = lone_lisp_list_rest(lone, arguments);

		} else if (!lone_lisp_is_nil(arguments)) {
			/* argument number mismatch: ((lambda (x) x) 10 20) */ linux_exit(-1);
		} else {
			/* matching number of arguments */
			break;
		}
	}

	return new_environment;
}

struct lone_lisp_machine_stack lone_lisp_machine_allocate_stack(struct lone_lisp *lone, size_t stack_size)
{
	struct lone_lisp_machine_stack stack;

	stack.base = lone_memory_array(lone->system, 0, 0, stack_size, sizeof(*stack.base), alignof(*stack.base));
	stack.limit = stack.base + stack_size;
	stack.top = stack.base;

	return stack;
}

void lone_lisp_machine_initialize(struct lone_lisp_machine *machine, struct lone_lisp_machine_stack stack)
{
	machine->stack = stack;
}

void lone_lisp_machine_reset(struct lone_lisp *lone, struct lone_lisp_machine *machine,
		struct lone_lisp_value module, struct lone_lisp_value expression)
{

	machine->step = LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION;
	lone_lisp_machine_push_step(lone, machine, LONE_LISP_MACHINE_STEP_HALT);
	machine->primitive.step = 0;

	machine->expression = expression;
	machine->module = module;
	machine->environment = lone_lisp_heap_value_of(lone, module)->as.module.environment;
}

bool lone_lisp_machine_cycle(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	struct lone_lisp_generator *generator;
	lone_lisp_integer count, i;

	switch (machine->step) {
	case LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION:
	expression_evaluation:
		switch (lone_lisp_type_of(machine->expression)) {
		case LONE_LISP_TAG_NIL:
		case LONE_LISP_TAG_FALSE:
		case LONE_LISP_TAG_TRUE:
		case LONE_LISP_TAG_INTEGER:
		case LONE_LISP_TAG_MODULE:
		case LONE_LISP_TAG_FUNCTION:
		case LONE_LISP_TAG_PRIMITIVE:
		case LONE_LISP_TAG_CONTINUATION:
		case LONE_LISP_TAG_GENERATOR:
		case LONE_LISP_TAG_VECTOR:
		case LONE_LISP_TAG_TABLE:
		case LONE_LISP_TAG_BYTES:
		case LONE_LISP_TAG_TEXT:
			machine->value = machine->expression;
			lone_lisp_machine_restore_step(lone, machine);
			break;
		case LONE_LISP_TAG_SYMBOL:
			machine->value = lone_lisp_table_get(lone, machine->environment, machine->expression);
			lone_lisp_machine_restore_step(lone, machine);
			break;
		case LONE_LISP_TAG_LIST:
			lone_lisp_machine_save_step(lone, machine);
			lone_lisp_machine_push_value(lone, machine, machine->environment);
			lone_lisp_machine_push_value(lone, machine, lone_lisp_list_rest(lone, machine->expression));
			machine->expression = lone_lisp_list_first(lone, machine->expression);
			lone_lisp_machine_push_step(lone, machine, LONE_LISP_MACHINE_STEP_EVALUATED_OPERATOR);
			goto expression_evaluation;
		}
		return true;
	case LONE_LISP_MACHINE_STEP_EVALUATED_OPERATOR:
		/* Evaluated operator is in machine->value.
		 * Stack:
		 * 	unevaluated-operands-list
		 * 	environment
		 * 	next-step
		 */
		machine->applicable = machine->value;
		machine->unevaluated = lone_lisp_machine_pop_value(lone, machine);
		machine->environment = lone_lisp_machine_pop_value(lone, machine);
		machine->list = lone_lisp_nil();
		switch (lone_lisp_type_of(machine->applicable)) {
		case LONE_LISP_TAG_FUNCTION:
		case LONE_LISP_TAG_PRIMITIVE:
		case LONE_LISP_TAG_CONTINUATION:
		case LONE_LISP_TAG_GENERATOR:
		case LONE_LISP_TAG_VECTOR:
		case LONE_LISP_TAG_TABLE:
			break;
		default:
			goto operator_not_applicable;
		}
		if (should_evaluate_operands(lone, machine->applicable, machine->unevaluated)) {
			lone_lisp_machine_push_value(lone, machine, machine->applicable);
			lone_lisp_machine_push_integer(lone, machine, 0); /* argument count */
			machine->step = LONE_LISP_MACHINE_STEP_OPERAND_EVALUATION;
		} else {
			machine->list = machine->unevaluated;
			machine->step = LONE_LISP_MACHINE_STEP_APPLICATION;
		}
		return true;
	case LONE_LISP_MACHINE_STEP_OPERAND_EVALUATION:
		/* Results of evaluation are pushed onto the stack.
		 * Remaining operands are in machine->unevaluated.
		 * Stack:
		 * 	argument-count
		 * 	arguments...
		 * 	applicable
		 * 	next-step
		 */
		machine->expression = lone_lisp_list_first(lone, machine->unevaluated);
		if (lone_lisp_list_has_rest(lone, machine->unevaluated)) {
			lone_lisp_machine_push_value(lone, machine, machine->unevaluated);
			lone_lisp_machine_push_value(lone, machine, machine->environment);
			lone_lisp_machine_push_step(lone, machine, LONE_LISP_MACHINE_STEP_OPERAND_ACCUMULATION);
		} else {
			/* Evlis tail recursion
			 * no new data is pushed onto the stack */
			lone_lisp_machine_push_step(lone, machine, LONE_LISP_MACHINE_STEP_LAST_OPERAND_ACCUMULATION);
		}
		goto expression_evaluation;
	case LONE_LISP_MACHINE_STEP_OPERAND_ACCUMULATION:
		/* Evaluated operand is in machine->value.
		 * Stack:
		 * 	environment
		 * 	unevaluated-operands-list
		 * 	argument-count
		 * 	arguments...
		 * 	applicable
		 * 	next-step
		 */
		machine->environment = lone_lisp_machine_pop_value(lone, machine);
		machine->unevaluated = lone_lisp_list_rest(lone, lone_lisp_machine_pop_value(lone, machine));
		count = lone_lisp_machine_pop_integer(lone, machine);
		lone_lisp_machine_push_value(lone, machine, machine->value);
		lone_lisp_machine_push_integer(lone, machine, count + 1);
		machine->step = LONE_LISP_MACHINE_STEP_OPERAND_EVALUATION;
		return true;
	case LONE_LISP_MACHINE_STEP_LAST_OPERAND_ACCUMULATION:
		/* Last evaluated operand is in machine->value.
		 * Rest are on the stack.
		 * Stack:
		 * 	argument-count
		 * 	arguments...
		 * 	applicable
		 * 	next-step
		 */
		count = lone_lisp_machine_pop_integer(lone, machine);
		machine->list = lone_lisp_list_create(lone, machine->value, lone_lisp_nil());
		for (i = 0; i < count; ++i) {
			machine->list =
				lone_lisp_list_create(
					lone,
					lone_lisp_machine_pop_value(lone, machine),
					machine->list
				);
		}
		machine->applicable = lone_lisp_machine_pop_value(lone, machine);
		machine->step = LONE_LISP_MACHINE_STEP_APPLICATION;
		return true;
	case LONE_LISP_MACHINE_STEP_APPLICATION:
		/* Operator is in machine->applicable.
		 * Operands, evaluated or not, are in machine->list.
		 * Stack:
		 * 	next-step
		 */
		switch (machine->applicable.tagged & LONE_LISP_TAG_MASK) {
		case LONE_LISP_TAG_FUNCTION:
			machine->environment = bind_arguments(
				lone,
				machine->environment,
				machine->applicable,
				machine->list
			);
			lone_lisp_machine_push_function_delimiter(lone, machine);
			machine->unevaluated = lone_lisp_heap_value_of(lone, machine->applicable)->as.function.code;
			machine->step = LONE_LISP_MACHINE_STEP_SEQUENCE_EVALUATION;
			return true;
		case LONE_LISP_TAG_PRIMITIVE:
			/* primitives pop the list of arguments from the stack */
			lone_lisp_machine_push_function_delimiter(lone, machine);
			lone_lisp_machine_push_value(lone, machine, machine->list);
			machine->primitive.step = 0;
		resume_primitive:
			machine->primitive.closure = lone_lisp_heap_value_of(lone, machine->applicable)->as.primitive.closure;
			machine->primitive.step =
				lone_lisp_heap_value_of(lone, machine->applicable)->as.primitive.function(
					lone,
					machine,
					machine->primitive.step
				);
			if (machine->primitive.step) {
				/* primitive did not finish, wants to be resumed
				 * may have saved data to stack and set machine
				 * to evaluate expression or apply function
				 * compute that then call primitive later with
				 * the returned step value so that primitive
				 * knows where to resume execution */
				lone_lisp_machine_save_primitive_step(lone, machine);
				lone_lisp_machine_push_value(lone, machine, machine->applicable);
				lone_lisp_machine_push_value(lone, machine, machine->environment);
				lone_lisp_machine_push_step(lone, machine, LONE_LISP_MACHINE_STEP_RESUME_PRIMITIVE);
			} else {
				/* primitives push the return value onto the stack */
				machine->value = lone_lisp_machine_pop_value(lone, machine);
				goto after_application;
			}
			return true;
		case LONE_LISP_TAG_CONTINUATION:
			if (lone_lisp_list_has_rest(lone, machine->list)) { goto too_many_arguments; }
			lone_lisp_machine_push_frames(
				lone,
				machine,
				lone_lisp_heap_value_of(lone, machine->applicable)->as.continuation.frame_count,
				lone_lisp_heap_value_of(lone, machine->applicable)->as.continuation.frames
			);
			lone_lisp_machine_restore_step(lone, machine);
			machine->value = lone_lisp_list_first(lone, machine->list);
			return true;
		case LONE_LISP_TAG_GENERATOR:
			generator = &lone_lisp_heap_value_of(lone, machine->applicable)->as.generator;
			if (!generator->stacks.own.top) { /* generator is finished */ linux_exit(-1); }
			if (generator->stacks.caller.base) { /* generator is already running */ linux_exit(-1); }
			generator->stacks.caller = machine->stack;
			if (generator->stacks.own.top == generator->stacks.own.base) {
				/* generator not yet started, initialize its stack */
				machine->stack = generator->stacks.own;
				lone_lisp_machine_push(lone, machine, (struct lone_lisp_machine_stack_frame) {
					.tagged =
						lone_lisp_retag(
							machine->applicable,
							LONE_LISP_TAG_GENERATOR_DELIMITER
						).tagged,
				});
				lone_lisp_machine_push_step(lone, machine, LONE_LISP_MACHINE_STEP_GENERATOR_RETURN);
				machine->expression = lone_lisp_list_create(lone, generator->function, machine->list);
				machine->step = LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION;
			} else {
				/* generator has executed before */
				if (lone_lisp_list_has_rest(lone, machine->list)) { goto too_many_arguments; }
				machine->stack = generator->stacks.own;
				machine->value = lone_lisp_list_first(lone, machine->list);
				machine->step = LONE_LISP_MACHINE_STEP_AFTER_APPLICATION;
			}
			return true;
		case LONE_LISP_TAG_VECTOR:
			machine->value = apply_to_vector(lone, machine->applicable, machine->list);
			lone_lisp_machine_restore_step(lone, machine);
			break;
		case LONE_LISP_TAG_TABLE:
			machine->value = apply_to_table(lone, machine->applicable, machine->list);
			lone_lisp_machine_restore_step(lone, machine);
			break;
		}
		lone_lisp_machine_restore_step(lone, machine);
		return true;
	case LONE_LISP_MACHINE_STEP_AFTER_APPLICATION:
		/* Result of function application is in machine->value.
		 * Stack:
		 * 	function-delimiter
		 * 	next-step
		 * 	next-step
		 */
	after_application:
		lone_lisp_machine_pop_function_delimiter(lone, machine);
		lone_lisp_machine_restore_step(lone, machine);
		lone_lisp_machine_restore_step(lone, machine);
		return true;
	case LONE_LISP_MACHINE_STEP_SEQUENCE_EVALUATION:
		/* Sequence is in machine->unevaluated.
		 * Stack:
		 * 	next-step
		 */
		machine->expression = lone_lisp_list_first(lone, machine->unevaluated);
		if (lone_lisp_list_has_rest(lone, machine->unevaluated)) {
			lone_lisp_machine_push_value(lone, machine, machine->environment);
			lone_lisp_machine_push_value(lone, machine, machine->unevaluated);
			lone_lisp_machine_push_step(lone, machine, LONE_LISP_MACHINE_STEP_SEQUENCE_EVALUATION_NEXT);
		} else {
			/* tail recursion optimization
			 * no new data is saved on the stack */
			lone_lisp_machine_push_step(lone, machine, LONE_LISP_MACHINE_STEP_AFTER_APPLICATION);
		}
		goto expression_evaluation;
	case LONE_LISP_MACHINE_STEP_SEQUENCE_EVALUATION_NEXT:
		/* Result of expression is in machine->value.
		 * Stack:
		 * 	unevaluated-expressions-list
		 * 	environment
		 * 	next-step
		 */
		machine->unevaluated = lone_lisp_list_rest(lone, lone_lisp_machine_pop_value(lone, machine));
		machine->environment = lone_lisp_machine_pop_value(lone, machine);
		machine->step = LONE_LISP_MACHINE_STEP_SEQUENCE_EVALUATION;
		return true;
	case LONE_LISP_MACHINE_STEP_SEQUENCE_EVALUATION_FROM_PRIMITIVE:
		/* Sequence is in machine->unevaluated.
		 * Stack:
		 * 	next-step
		 */
		machine->expression = lone_lisp_list_first(lone, machine->unevaluated);
		if (lone_lisp_list_has_rest(lone, machine->unevaluated)) {
			lone_lisp_machine_push_value(lone, machine, machine->environment);
			lone_lisp_machine_push_value(lone, machine, machine->unevaluated);
			lone_lisp_machine_push_step(lone, machine,
					LONE_LISP_MACHINE_STEP_SEQUENCE_EVALUATION_FROM_PRIMITIVE_NEXT);
		} else {
			/* tail recursion optimization
			 * nothing is saved on the stack */
		}
		goto expression_evaluation;
	case LONE_LISP_MACHINE_STEP_SEQUENCE_EVALUATION_FROM_PRIMITIVE_NEXT:
		/* Result of expression is in machine->value.
		 * Stack:
		 * 	unevaluated-expressions-list
		 * 	environment
		 * 	next-step
		 */
		machine->unevaluated = lone_lisp_list_rest(lone, lone_lisp_machine_pop_value(lone, machine));
		machine->environment = lone_lisp_machine_pop_value(lone, machine);
		machine->step = LONE_LISP_MACHINE_STEP_SEQUENCE_EVALUATION_FROM_PRIMITIVE;
		return true;
	case LONE_LISP_MACHINE_STEP_RESUME_PRIMITIVE:
		/* Stack:
		 * 	environment
		 * 	primitive
		 * 	primitive-step
		 * 	primitive-data...
		 * 	function-delimiter
		 * 	next-step
		 */
		machine->environment = lone_lisp_machine_pop_value(lone, machine);
		machine->applicable = lone_lisp_machine_pop_value(lone, machine);
		lone_lisp_machine_restore_primitive_step(lone, machine);
		goto resume_primitive;
	case LONE_LISP_MACHINE_STEP_GENERATOR_RETURN:
		/* Stack:
		 * 	generator-delimiter
		 */
		generator = &lone_lisp_heap_value_of(
			lone,
			lone_lisp_retag_frame(machine->stack.base[0], LONE_LISP_TAG_GENERATOR)
		)->as.generator;
		generator->stacks.own.top = 0; /* generator has finished */
		machine->stack = generator->stacks.caller;
		generator->stacks.caller = (struct lone_lisp_machine_stack) { 0 };
		lone_lisp_machine_restore_step(lone, machine);
		lone_lisp_machine_restore_step(lone, machine);
		return true;
	case LONE_LISP_MACHINE_STEP_HALT:
		return false;
	}

too_many_arguments:
operator_not_applicable:
	linux_exit(-1);
}
