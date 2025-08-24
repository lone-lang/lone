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
