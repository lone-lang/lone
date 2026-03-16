/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/garbage_collector.h>
#include <lone/lisp/heap.h>

#include <lone/memory/allocator.h>

#include <lone/architecture/garbage_collector.c>

static void lone_lisp_mark_heap_value(struct lone_lisp *lone, struct lone_lisp_heap_value *value);
static void lone_lisp_mark_lisp_stack_values(struct lone_lisp *lone,
		struct lone_lisp_machine_stack_frame *base, struct lone_lisp_machine_stack_frame *limit);

static void lone_lisp_mark_value(struct lone_lisp *lone, struct lone_lisp_value value)
{
	switch (lone_lisp_type_of(value)) {
	case LONE_LISP_TYPE_NIL:
	case LONE_LISP_TYPE_FALSE:
	case LONE_LISP_TYPE_TRUE:
	case LONE_LISP_TYPE_INTEGER:
		/* value types need not be marked */
		return;
	case LONE_LISP_TYPE_HEAP_VALUE:
		break;
	}

	lone_lisp_mark_heap_value(lone, lone_lisp_heap_value_of(lone, value));
}

static void lone_lisp_mark_heap_value(struct lone_lisp *lone, struct lone_lisp_heap_value *value)
{
	if (!value || !value->live || value->marked) { return; }

	value->marked = true;

	switch (value->type) {
	case LONE_LISP_TYPE_MODULE:
		lone_lisp_mark_value(lone, value->as.module.name);
		lone_lisp_mark_value(lone, value->as.module.environment);
		lone_lisp_mark_value(lone, value->as.module.exports);
		break;
	case LONE_LISP_TYPE_FUNCTION:
		lone_lisp_mark_value(lone, value->as.function.arguments);
		lone_lisp_mark_value(lone, value->as.function.code);
		lone_lisp_mark_value(lone, value->as.function.environment);
		break;
	case LONE_LISP_TYPE_PRIMITIVE:
		lone_lisp_mark_value(lone, value->as.primitive.name);
		lone_lisp_mark_value(lone, value->as.primitive.closure);
		break;
	case LONE_LISP_TYPE_CONTINUATION:
		lone_lisp_mark_lisp_stack_values(
			lone,
			value->as.continuation.frames,
			value->as.continuation.frames + value->as.continuation.frame_count
		);
		break;
	case LONE_LISP_TYPE_LIST:
		lone_lisp_mark_value(lone, value->as.list.first);
		lone_lisp_mark_value(lone, value->as.list.rest);
		break;
	case LONE_LISP_TYPE_VECTOR:
		for (size_t i = 0; i < value->as.vector.count; ++i) {
			lone_lisp_mark_value(lone, value->as.vector.values[i]);
		}
		break;
	case LONE_LISP_TYPE_TABLE:
		lone_lisp_mark_value(lone, value->as.table.prototype);
		for (size_t i = 0; i < value->as.table.count; ++i) {
			lone_lisp_mark_value(lone, value->as.table.entries[i].key);
			lone_lisp_mark_value(lone, value->as.table.entries[i].value);
		}
		break;
	case LONE_LISP_TYPE_SYMBOL:
	case LONE_LISP_TYPE_TEXT:
	case LONE_LISP_TYPE_BYTES:
		/* these types do not contain any other values to mark */
		break;
	}
}

static void lone_lisp_mark_known_roots(struct lone_lisp *lone)
{
	lone_lisp_mark_value(lone, lone->symbol_table);
	lone_lisp_mark_value(lone, lone->modules.loaded);
	lone_lisp_mark_value(lone, lone->modules.embedded);
	lone_lisp_mark_value(lone, lone->modules.null);
	lone_lisp_mark_value(lone, lone->modules.top_level_environment);
	lone_lisp_mark_value(lone, lone->modules.path);
}

static bool lone_points_within_range(void *pointer, void *start, void *end)
{
	return start <= pointer && pointer < end;
}

static bool lone_lisp_points_to_heap(struct lone_lisp *lone, void *pointer)
{
	return lone_points_within_range(
		pointer,
		lone->heap.values,
		lone->heap.values + lone->heap.count
	);
}

static void lone_lisp_mark_stack_roots(struct lone_lisp *lone, void *bottom, void *top)
{
	void *tmp, **pointer;

	if (top < bottom) {
		tmp = bottom;
		bottom = top;
		top = tmp;
	}

	pointer = bottom;

	while (pointer++ < top) {
		if (lone_lisp_points_to_heap(lone, *pointer)) {
			lone_lisp_mark_heap_value(lone, *pointer);
		}
	}
}

static void lone_lisp_mark_native_stack_roots(struct lone_lisp *lone)
{
	lone_lisp_mark_stack_roots(
		lone,
		lone->native_stack,
		__builtin_frame_address(0)
	);
}

static void lone_lisp_mark_lisp_stack_values(struct lone_lisp *lone,
		struct lone_lisp_machine_stack_frame *base, struct lone_lisp_machine_stack_frame *limit)
{
	struct lone_lisp_machine_stack_frame *frame = base;

	while (frame++ < limit) {
		switch (frame->type) {
		case LONE_LISP_MACHINE_STACK_FRAME_TYPE_INTEGER:
		case LONE_LISP_MACHINE_STACK_FRAME_TYPE_STEP:
		case LONE_LISP_MACHINE_STACK_FRAME_TYPE_PRIMITIVE_STEP:
			continue;
		case LONE_LISP_MACHINE_STACK_FRAME_TYPE_VALUE:
		case LONE_LISP_MACHINE_STACK_FRAME_TYPE_FUNCTION_DELIMITER:
		case LONE_LISP_MACHINE_STACK_FRAME_TYPE_CONTINUATION_DELIMITER:
			lone_lisp_mark_value(lone, frame->as.value);
		}
	}
}

static void lone_lisp_mark_lisp_stack_roots_of(struct lone_lisp *lone, struct lone_lisp_machine_stack stack)
{
	lone_lisp_mark_lisp_stack_values(lone, stack.base, stack.top);
}

static void lone_lisp_mark_lisp_stack_roots(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	lone_lisp_mark_lisp_stack_roots_of(lone, machine->stack);
}

static void lone_lisp_mark_all_reachable_values(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	lone_registers registers;          /* stack space for registers */
	lone_save_registers(registers);    /* spill registers on stack */

	/* precise */
	lone_lisp_mark_known_roots(lone);
	lone_lisp_mark_lisp_stack_roots(lone, machine);

	/* conservative */
	lone_lisp_mark_native_stack_roots(lone);
}

static void lone_lisp_kill_all_unmarked_values(struct lone_lisp *lone)
{
	struct lone_lisp_heap_value *value;
	size_t i;

	for (i = 0; i < lone->heap.count; ++i) {
		value = &lone->heap.values[i];

		if (!value->live) { continue; }

		if (!value->marked) {

			switch (value->type) {
			case LONE_LISP_TYPE_BYTES:
			case LONE_LISP_TYPE_TEXT:
			case LONE_LISP_TYPE_SYMBOL:
				if (value->should_deallocate_bytes) {
					lone_deallocate(lone->system, value->as.bytes.pointer);
				}
				break;
			case LONE_LISP_TYPE_VECTOR:
				lone_deallocate(lone->system, value->as.vector.values);
				break;
			case LONE_LISP_TYPE_TABLE:
				lone_deallocate(lone->system, value->as.table.indexes);
				lone_deallocate(lone->system, value->as.table.entries);
				break;
			case LONE_LISP_TYPE_CONTINUATION:
				lone_deallocate(lone->system, value->as.continuation.frames);
				break;
			case LONE_LISP_TYPE_MODULE:
			case LONE_LISP_TYPE_FUNCTION:
			case LONE_LISP_TYPE_PRIMITIVE:
			case LONE_LISP_TYPE_LIST:
				/* these types do not own any additional memory */
				break;
			}

			value->live = false;
		}

		value->marked = false;
	}
}

void lone_lisp_garbage_collector(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	lone_lisp_mark_all_reachable_values(lone, machine);
	lone_lisp_kill_all_unmarked_values(lone);
}
