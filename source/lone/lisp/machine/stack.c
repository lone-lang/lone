/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/definitions.h>
#include <lone/lisp/types.h>
#include <lone/lisp/machine/stack.h>

#include <lone/memory/functions.h>
#include <lone/memory/array.h>

#include <lone/stack.h>
#include <lone/linux.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Low-level mmap helpers for stack memory management.                 │
   │    These wrap the Linux syscall parameters for anonymous RW mappings.  │
   │    Replaceable with arena allocation in the concurrent processing      │
   │    phase without changing any consumer code.                           │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

static intptr_t lone_lisp_machine_stack_mmap(size_t size)
{
	return linux_mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

static intptr_t lone_lisp_machine_stack_mremap(void *address, size_t old_size, size_t new_size)
{
	return linux_mremap(address, old_size, new_size, MREMAP_MAYMOVE, 0);
}

static void lone_lisp_machine_stack_munmap(void *address, size_t size)
{
	linux_munmap(address, size);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Stack allocation and deallocation.                                  │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_lisp_machine_stack lone_lisp_machine_allocate_stack(struct lone_lisp *lone, size_t count)
{
	struct lone_lisp_machine_stack stack;
	size_t size;
	intptr_t result;

	if (__builtin_mul_overflow(count, sizeof(*stack.base), &size)) { linux_exit(-1); }

	result = lone_lisp_machine_stack_mmap(size);
	if (result < 0) { linux_exit(-1); }

	stack.base = (void *) result;
	stack.top = stack.base;
	stack.limit = stack.base + count;

	return stack;
}

void lone_lisp_machine_deallocate_stack(struct lone_lisp *lone, struct lone_lisp_machine_stack stack)
{
	size_t count, size;

	count = stack.limit - stack.base;
	if (__builtin_mul_overflow(count, sizeof(*stack.base), &size)) { linux_exit(-1); }

	lone_lisp_machine_stack_munmap(stack.base, size);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Stack growth and shrinking.                                         │
   │                                                                        │
   │    Growth: double via mremap on push overflow,                         │
   │    capped at LONE_LISP_MACHINE_STACK_MAXIMUM_SIZE.                     │
   │                                                                        │
   │    Shrinking: halve at reset when capacity >= 8x initial.              │
   │    Gentle policy to avoid grow-shrink thrashing.                       │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

static bool lone_lisp_machine_grow_stack(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	struct lone_lisp_machine_stack_frame *old_base;
	size_t old_count, new_count, old_size, new_size, top_offset;
	intptr_t result;

	old_base = machine->stack.base;
	old_count = machine->stack.limit - machine->stack.base;
	top_offset = machine->stack.top - machine->stack.base;

	if (__builtin_mul_overflow(old_count, LONE_LISP_MACHINE_STACK_GROWTH_FACTOR, &new_count))
		goto overflow;

	if (new_count > LONE_LISP_MACHINE_STACK_MAXIMUM_SIZE)
		goto overflow;

	if (__builtin_mul_overflow(old_count, sizeof(*old_base), &old_size))
		goto overflow;

	if (__builtin_mul_overflow(new_count, sizeof(*old_base), &new_size))
		goto overflow;

	result = lone_lisp_machine_stack_mremap(old_base, old_size, new_size);
	if (result < 0)
		return false;

	machine->stack.base  = (void *) result;
	machine->stack.top   = machine->stack.base + top_offset;
	machine->stack.limit = machine->stack.base + new_count;

	return true;

overflow:
	return false;
}

void lone_lisp_machine_shrink_stack(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	size_t capacity, threshold, new_count, old_size, new_size;
	intptr_t result;

	capacity = machine->stack.limit - machine->stack.base;

	if (__builtin_mul_overflow(machine->initial_stack_count,
		LONE_LISP_MACHINE_STACK_SHRINK_THRESHOLD, &threshold))
		goto overflow;

	if (capacity < threshold)
		return;

	new_count = capacity / LONE_LISP_MACHINE_STACK_GROWTH_FACTOR;

	if (__builtin_mul_overflow(capacity, sizeof(*machine->stack.base), &old_size))
		goto overflow;

	if (__builtin_mul_overflow(new_count, sizeof(*machine->stack.base), &new_size))
		goto overflow;

	result = lone_lisp_machine_stack_mremap(machine->stack.base, old_size, new_size);
	if (result < 0)
		return;

	machine->stack.base  = (void *) result;
	machine->stack.top   = machine->stack.base;
	machine->stack.limit = machine->stack.base + new_count;

	return;

overflow:
	return;
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Stack push, pop, and peek operations.                               │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

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
	if (!lone_lisp_machine_can_push(machine)) {
		if (!lone_lisp_machine_grow_stack(lone, machine)) { linux_exit(-1); }
	}
	*machine->stack.top++ = frame;
}

void lone_lisp_machine_push_frames(struct lone_lisp *lone, struct lone_lisp_machine *machine,
		size_t frame_count, struct lone_lisp_machine_stack_frame *frames)
{
	size_t size = lone_memory_array_size_in_bytes(frame_count, sizeof(*frames));
	if (!lone_lisp_machine_can_push_bytes(machine, size)) {
		if (!lone_lisp_machine_grow_stack(lone, machine)) { linux_exit(-1); }
	}
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

void lone_lisp_machine_push_function_delimiter(struct lone_lisp *lone, struct lone_lisp_machine *machine,
		struct lone_lisp_value function)
{
	lone_lisp_machine_push(lone, machine, (struct lone_lisp_machine_stack_frame) {
		.tagged = lone_lisp_retag(function, LONE_LISP_TAG_FUNCTION_DELIMITER).tagged,
	});
}

void lone_lisp_machine_pop_function_delimiter(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	lone_lisp_machine_pop(lone, machine);
}

void lone_lisp_machine_push_primitive_delimiter(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	lone_lisp_machine_push(lone, machine, (struct lone_lisp_machine_stack_frame) {
		.tagged = LONE_LISP_TAG_PRIMITIVE_DELIMITER,
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

void lone_lisp_machine_pop_primitive_delimiter(struct lone_lisp *lone, struct lone_lisp_machine *machine)
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

void lone_lisp_machine_unwind_to_primitive_delimiter(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	lone_lisp_machine_unwind_to(lone, machine, LONE_LISP_TAG_PRIMITIVE_DELIMITER);
}

void lone_lisp_machine_unwind_to_interceptor_delimiter(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	lone_lisp_machine_unwind_to(lone, machine, LONE_LISP_TAG_INTERCEPTOR_DELIMITER);
}
