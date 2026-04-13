/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/types.h>
#include <lone/lisp/machine/stack.h>

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

struct lone_lisp_machine_stack lone_lisp_machine_allocate_stack(struct lone_lisp *lone, size_t stack_size)
{
	struct lone_lisp_machine_stack stack;

	stack.base = lone_memory_array(lone->system, 0, 0, stack_size, sizeof(*stack.base), alignof(*stack.base));
	stack.limit = stack.base + stack_size;
	stack.top = stack.base;

	return stack;
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
