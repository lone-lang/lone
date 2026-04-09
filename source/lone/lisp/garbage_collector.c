/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/garbage_collector.h>
#include <lone/lisp/heap.h>

#include <lone/memory/allocator.h>
#include <lone/memory/functions.h>

#include <lone/bits.h>

#include <lone/architecture/garbage_collector.c>

#include <limits.h>

static void lone_lisp_mark_heap_value(struct lone_lisp *lone, struct lone_lisp_heap_value *value);
static void lone_lisp_mark_lisp_stack_values(struct lone_lisp *lone,
		struct lone_lisp_machine_stack_frame *base, struct lone_lisp_machine_stack_frame *limit);
static void lone_lisp_mark_lisp_stack_roots_of(struct lone_lisp *lone, struct lone_lisp_machine_stack stack);

static void lone_lisp_mark_value(struct lone_lisp *lone, struct lone_lisp_value value)
{
	if (value.tagged & 1) {
		/* non-heap value: integer, nil, true, false */
		return;
	}

	lone_lisp_mark_heap_value(lone, lone_lisp_heap_value_of(lone, value));
}

static void lone_lisp_mark_heap_value(struct lone_lisp *lone, struct lone_lisp_heap_value *value)
{
	size_t index;

	if (!value) { return; }

	index = value - lone->heap.values;

	if (!lone_bits_get(lone->heap.bits.live, index)) { return; }
	if (lone_bits_get(lone->heap.bits.marked, index)) { return; }

	lone_bits_mark(lone->heap.bits.marked, index);

	switch (value->type) {
	case LONE_LISP_TAG_MODULE:
		lone_lisp_mark_value(lone, value->as.module.name);
		lone_lisp_mark_value(lone, value->as.module.environment);
		lone_lisp_mark_value(lone, value->as.module.exports);
		break;
	case LONE_LISP_TAG_FUNCTION:
		lone_lisp_mark_value(lone, value->as.function.arguments);
		lone_lisp_mark_value(lone, value->as.function.code);
		lone_lisp_mark_value(lone, value->as.function.environment);
		break;
	case LONE_LISP_TAG_PRIMITIVE:
		lone_lisp_mark_value(lone, value->as.primitive.name);
		lone_lisp_mark_value(lone, value->as.primitive.closure);
		break;
	case LONE_LISP_TAG_CONTINUATION:
		lone_lisp_mark_lisp_stack_values(
			lone,
			value->as.continuation.frames,
			value->as.continuation.frames + value->as.continuation.frame_count
		);
		break;
	case LONE_LISP_TAG_GENERATOR:
		lone_lisp_mark_value(lone, value->as.generator.function);
		if (value->as.generator.stacks.caller.base) {
			/* generator is running */
			lone_lisp_mark_lisp_stack_roots_of(lone, value->as.generator.stacks.caller);
		} else {
			/* generator is not running */
			if (value->as.generator.stacks.own.top) {
				/* generator is suspended */
				lone_lisp_mark_lisp_stack_roots_of(lone, value->as.generator.stacks.own);
			} else {
				/* generator is finished */
			}
		}
		break;
	case LONE_LISP_TAG_LIST:
		lone_lisp_mark_value(lone, value->as.list.first);
		lone_lisp_mark_value(lone, value->as.list.rest);
		break;
	case LONE_LISP_TAG_VECTOR:
		for (size_t i = 0; i < value->as.vector.count; ++i) {
			lone_lisp_mark_value(lone, value->as.vector.values[i]);
		}
		break;
	case LONE_LISP_TAG_TABLE:
		lone_lisp_mark_value(lone, value->as.table.prototype);
		for (size_t i = 0; i < value->as.table.count; ++i) {
			lone_lisp_mark_value(lone, value->as.table.entries[i].key);
			lone_lisp_mark_value(lone, value->as.table.entries[i].value);
		}
		break;
	case LONE_LISP_TAG_SYMBOL:
	case LONE_LISP_TAG_TEXT:
	case LONE_LISP_TAG_BYTES:
		/* these types do not contain any other values to mark */
		break;
	}
}

static void lone_lisp_pin_and_mark_heap_value(struct lone_lisp *lone, struct lone_lisp_heap_value *value)
{
	size_t index;

	if (!value) { return; }
	index = value - lone->heap.values;
	if (!lone_bits_get(lone->heap.bits.live, index)) { return; }

	lone_bits_mark(lone->heap.bits.pinned, index);
	lone_lisp_mark_heap_value(lone, value);
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
	return pointer >= start && pointer < end;
}

static bool lone_lisp_points_to_heap(struct lone_lisp *lone, void *pointer)
{
	return lone_points_within_range(
		pointer,
		lone->heap.values,
		lone->heap.values + lone->heap.count
	);
}

static void lone_lisp_mark_native_stack_roots_in_range(struct lone_lisp *lone, void *bottom, void *top)
{
	void *tmp, **pointer;
	unsigned long word;
	size_t index;

	if (top < bottom) {
		tmp = bottom;
		bottom = top;
		top = tmp;
	}

	for (pointer = bottom; pointer < top; ++pointer) {
		if (lone_lisp_points_to_heap(lone, *pointer)) {
			lone_lisp_pin_and_mark_heap_value(lone, *pointer);
		}

		word = (unsigned long) *pointer;

		if (!(word & 1)) {
			index = word >> LONE_LISP_INDEX_SHIFT;

			if (index < lone->heap.count) {
				lone_lisp_pin_and_mark_heap_value(lone, &lone->heap.values[index]);
			}
		}
	}
}

static void lone_lisp_mark_native_stack_roots(struct lone_lisp *lone)
{
	lone_lisp_mark_native_stack_roots_in_range(
		lone,
		lone->native_stack,
		__builtin_frame_address(0)
	);
}

static void lone_lisp_mark_lisp_stack_values(struct lone_lisp *lone,
		struct lone_lisp_machine_stack_frame *base, struct lone_lisp_machine_stack_frame *limit)
{
	struct lone_lisp_machine_stack_frame *frame;

	for (frame = base; frame < limit; ++frame) {
		switch (frame->type) {
		case LONE_LISP_MACHINE_STACK_FRAME_TYPE_INTEGER:
		case LONE_LISP_MACHINE_STACK_FRAME_TYPE_STEP:
		case LONE_LISP_MACHINE_STACK_FRAME_TYPE_PRIMITIVE_STEP:
			continue;
		case LONE_LISP_MACHINE_STACK_FRAME_TYPE_VALUE:
		case LONE_LISP_MACHINE_STACK_FRAME_TYPE_FUNCTION_DELIMITER:
		case LONE_LISP_MACHINE_STACK_FRAME_TYPE_CONTINUATION_DELIMITER:
		case LONE_LISP_MACHINE_STACK_FRAME_TYPE_GENERATOR_DELIMITER:
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
	size_t first_dead, last_live, i;

	first_dead = lone->heap.count;
	last_live = 0;

	for (i = 0; i < lone->heap.count; ++i) {

		if (!lone_bits_get(lone->heap.bits.live, i)) {
			if (i < first_dead) { first_dead = i; }
			continue;
		}

		if (!lone_bits_get(lone->heap.bits.marked, i)) {

			value = &lone->heap.values[i];

			switch (value->type) {
			case LONE_LISP_TAG_BYTES:
			case LONE_LISP_TAG_TEXT:
				if (value->should_deallocate_bytes) {
					lone_memory_deallocate(
						lone->system, value->as.bytes.pointer,
						value->as.bytes.count + 1,
						1, 1
					);
				}
				break;
			case LONE_LISP_TAG_SYMBOL:
				if (value->should_deallocate_bytes) {
					lone_memory_deallocate(
						lone->system,
						value->as.symbol.name.pointer,
						value->as.symbol.name.count + 1,
						1, 1
					);
				}
				break;
			case LONE_LISP_TAG_VECTOR:
				lone_memory_deallocate(
					lone->system, value->as.vector.values,
					value->as.vector.capacity,
					sizeof(*value->as.vector.values), alignof(*value->as.vector.values)
				);
				break;
			case LONE_LISP_TAG_TABLE:
				lone_memory_deallocate(
					lone->system, value->as.table.indexes,
					value->as.table.capacity,
					sizeof(*value->as.table.indexes), alignof(*value->as.table.indexes)
				);
				lone_memory_deallocate(
					lone->system, value->as.table.entries,
					value->as.table.capacity,
					sizeof(*value->as.table.entries), alignof(*value->as.table.entries)
				);
				break;
			case LONE_LISP_TAG_CONTINUATION:
				lone_memory_deallocate(
					lone->system, value->as.continuation.frames,
					value->as.continuation.frame_count,
					sizeof(*value->as.continuation.frames), alignof(*value->as.continuation.frames)
				);
				break;
			case LONE_LISP_TAG_GENERATOR:
				lone_memory_deallocate(
					lone->system, value->as.generator.stacks.own.base,
					value->as.generator.stacks.own.limit - value->as.generator.stacks.own.base,
					sizeof(*value->as.generator.stacks.own.base),
					alignof(*value->as.generator.stacks.own.base)
				);
				break;
			case LONE_LISP_TAG_MODULE:
			case LONE_LISP_TAG_FUNCTION:
			case LONE_LISP_TAG_PRIMITIVE:
			case LONE_LISP_TAG_LIST:
				/* these types do not own any additional memory */
				break;
			}

			lone_bits_clear(lone->heap.bits.live, i);
			if (i < first_dead) { first_dead = i; }
		} else {
			last_live = i;
		}
	}

	lone_memory_zero(lone->heap.bits.marked, lone_lisp_heap_bitmap_size(lone->heap.capacity));

	lone->heap.first_dead = first_dead;
	if (last_live == 0 && !lone_bits_get(lone->heap.bits.live, 0)) {
		lone->heap.count = 0;
	} else if (last_live + 1 < lone->heap.count) {
		lone->heap.count = last_live + 1;
	}
}

static struct lone_optional_size lone_lisp_find_first_dead(struct lone_lisp *lone, size_t start)
{
	struct lone_optional_size index;
	unsigned char *bits;
	size_t bitmap_bytes, byte_offset;

	if (start >= lone->heap.count) {
		return LONE_OPTIONAL_ABSENT_VALUE(size);
	}

	bitmap_bytes = (lone->heap.count + CHAR_BIT - 1) / CHAR_BIT;
	byte_offset = start / CHAR_BIT;
	bits = ((unsigned char *) lone->heap.bits.live) + byte_offset;

	index = lone_bits_find_first_zero(bits, bitmap_bytes - byte_offset);
	if (index.present) {
		index.value = byte_offset * CHAR_BIT + index.value;
	}

	return index;
}

static bool lone_lisp_is_alive(struct lone_lisp *lone, size_t index)
{
	return lone_bits_get(lone->heap.bits.live, index);
}

static bool lone_lisp_is_pinned(struct lone_lisp *lone, size_t index)
{
	return lone_bits_get(lone->heap.bits.pinned, index);
}

static bool lone_lisp_is_moveable(struct lone_lisp *lone, size_t index)
{
	return lone_lisp_is_alive(lone, index) && !lone_lisp_is_pinned(lone, index);
}

static void lone_lisp_move_heap_value(struct lone_lisp *lone, size_t from, size_t to)
{
	lone->heap.values[to] = lone->heap.values[from];

	lone_bits_mark(lone->heap.bits.live, to);
	lone_bits_clear(lone->heap.bits.live, from);

	lone->heap.values[from].as.metadata.forwarding_index = to;
}

static struct lone_lisp_value lone_lisp_forward_value(struct lone_lisp *lone, struct lone_lisp_value value)
{
	size_t old_index, new_index;
	long preserved;

	if (!lone_lisp_is_heap_value(value)) { return value; }

	old_index = ((unsigned long) value.tagged) >> LONE_LISP_INDEX_SHIFT;

	if (old_index >= lone->heap.count) { return value; }

	if (lone_lisp_is_alive(lone, old_index)) { return value; }

	new_index = lone->heap.values[old_index].as.metadata.forwarding_index;

	/* preserve tag and metadata bits, replace the index */
	preserved = value.tagged & (((long) 1 << LONE_LISP_INDEX_SHIFT) - 1);

	return (struct lone_lisp_value) {
		.tagged = preserved | (long) ((unsigned long) new_index << LONE_LISP_INDEX_SHIFT),
	};
}

static void lone_lisp_rewrite_stack_frames(struct lone_lisp *lone,
		struct lone_lisp_machine_stack_frame *base, struct lone_lisp_machine_stack_frame *limit)
{
	struct lone_lisp_machine_stack_frame *frame;

	for (frame = base; frame < limit; ++frame) {
		switch (frame->type) {
		case LONE_LISP_MACHINE_STACK_FRAME_TYPE_INTEGER:
		case LONE_LISP_MACHINE_STACK_FRAME_TYPE_STEP:
		case LONE_LISP_MACHINE_STACK_FRAME_TYPE_PRIMITIVE_STEP:
			continue;
		case LONE_LISP_MACHINE_STACK_FRAME_TYPE_VALUE:
		case LONE_LISP_MACHINE_STACK_FRAME_TYPE_FUNCTION_DELIMITER:
		case LONE_LISP_MACHINE_STACK_FRAME_TYPE_CONTINUATION_DELIMITER:
		case LONE_LISP_MACHINE_STACK_FRAME_TYPE_GENERATOR_DELIMITER:
			frame->as.value = lone_lisp_forward_value(lone, frame->as.value);
			break;
		}
	}
}

static void lone_lisp_rewrite_heap_value_interior(struct lone_lisp *lone, struct lone_lisp_heap_value *value)
{
	switch (value->type) {
	case LONE_LISP_TAG_MODULE:
		value->as.module.name = lone_lisp_forward_value(lone, value->as.module.name);
		value->as.module.environment = lone_lisp_forward_value(lone, value->as.module.environment);
		value->as.module.exports = lone_lisp_forward_value(lone, value->as.module.exports);
		break;
	case LONE_LISP_TAG_FUNCTION:
		value->as.function.arguments = lone_lisp_forward_value(lone, value->as.function.arguments);
		value->as.function.code = lone_lisp_forward_value(lone, value->as.function.code);
		value->as.function.environment = lone_lisp_forward_value(lone, value->as.function.environment);
		break;
	case LONE_LISP_TAG_PRIMITIVE:
		value->as.primitive.name = lone_lisp_forward_value(lone, value->as.primitive.name);
		value->as.primitive.closure = lone_lisp_forward_value(lone, value->as.primitive.closure);
		break;
	case LONE_LISP_TAG_CONTINUATION:
		lone_lisp_rewrite_stack_frames(
			lone,
			value->as.continuation.frames,
			value->as.continuation.frames + value->as.continuation.frame_count
		);
		break;
	case LONE_LISP_TAG_GENERATOR:
		value->as.generator.function = lone_lisp_forward_value(lone, value->as.generator.function);
		if (value->as.generator.stacks.caller.base) {
			lone_lisp_rewrite_stack_frames(
				lone,
				value->as.generator.stacks.caller.base,
				value->as.generator.stacks.caller.top
			);
		} else if (value->as.generator.stacks.own.top) {
			lone_lisp_rewrite_stack_frames(
				lone,
				value->as.generator.stacks.own.base,
				value->as.generator.stacks.own.top
			);
		}
		break;
	case LONE_LISP_TAG_LIST:
		value->as.list.first = lone_lisp_forward_value(lone, value->as.list.first);
		value->as.list.rest = lone_lisp_forward_value(lone, value->as.list.rest);
		break;
	case LONE_LISP_TAG_VECTOR:
		for (size_t i = 0; i < value->as.vector.count; ++i) {
			value->as.vector.values[i] = lone_lisp_forward_value(lone, value->as.vector.values[i]);
		}
		break;
	case LONE_LISP_TAG_TABLE:
		value->as.table.prototype = lone_lisp_forward_value(lone, value->as.table.prototype);
		for (size_t i = 0; i < value->as.table.count; ++i) {
			value->as.table.entries[i].key = lone_lisp_forward_value(lone, value->as.table.entries[i].key);
			value->as.table.entries[i].value = lone_lisp_forward_value(lone, value->as.table.entries[i].value);
		}
		break;
	case LONE_LISP_TAG_SYMBOL:
	case LONE_LISP_TAG_TEXT:
	case LONE_LISP_TAG_BYTES:
		break;
	}
}

static void lone_lisp_rewrite_all_references(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	/* known roots */
	lone->symbol_table = lone_lisp_forward_value(lone, lone->symbol_table);
	lone->modules.loaded = lone_lisp_forward_value(lone, lone->modules.loaded);
	lone->modules.embedded = lone_lisp_forward_value(lone, lone->modules.embedded);
	lone->modules.null = lone_lisp_forward_value(lone, lone->modules.null);
	lone->modules.top_level_environment = lone_lisp_forward_value(lone, lone->modules.top_level_environment);
	lone->modules.path = lone_lisp_forward_value(lone, lone->modules.path);

	/* lisp machine registers */
	machine->value = lone_lisp_forward_value(lone, machine->value);
	machine->environment = lone_lisp_forward_value(lone, machine->environment);
	machine->expression = lone_lisp_forward_value(lone, machine->expression);
	machine->applicable = lone_lisp_forward_value(lone, machine->applicable);
	machine->unevaluated = lone_lisp_forward_value(lone, machine->unevaluated);
	machine->list = lone_lisp_forward_value(lone, machine->list);
	machine->module = lone_lisp_forward_value(lone, machine->module);
	machine->primitive.closure = lone_lisp_forward_value(lone, machine->primitive.closure);

	/* lisp machine stack */
	lone_lisp_rewrite_stack_frames(lone, machine->stack.base, machine->stack.top);

	/* interior of every live heap value */
	for (size_t i = 0; i < lone->heap.count; ++i) {
		if (!lone_lisp_is_alive(lone, i)) { continue; }
		lone_lisp_rewrite_heap_value_interior(lone, &lone->heap.values[i]);
	}
}

static void lone_lisp_recalculate_heap_bounds(struct lone_lisp *lone)
{
	struct lone_optional_size first;
	size_t new_count;

	first = lone_lisp_find_first_dead(lone, 0);
	lone->heap.first_dead = first.present? first.value : lone->heap.count;

	new_count = lone->heap.count;
	while (new_count > 0 && !lone_lisp_is_alive(lone, new_count - 1)) {
		--new_count;
	}
	lone->heap.count = new_count;
}

static void lone_lisp_compact_heap(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	struct lone_optional_size index;
	size_t low, high;
	bool moved;

	low = lone->heap.first_dead; /* scans up for dead values */
	high = lone->heap.count; /* scans down for unpinned live values */
	moved = false;

	if ((low == 0 && high == 0) || low >= high) { goto zero_pinned; }

	--high;

	while (low < high) {

		/* find next dead slot from the bottom */
		index = lone_lisp_find_first_dead(lone, low);
		if (!index.present) { /* no dead values to compact into */ break; }
		low = index.value;

		/* find next live unpinned value from the top */
		while (low < high && !lone_lisp_is_moveable(lone, high)) {
			--high;
		}

		if (low < high) {
			lone_lisp_move_heap_value(lone, high, low);
			moved = true;
			++low;
			--high;
		}
	}

	if (moved) {
		lone_lisp_rewrite_all_references(lone, machine);
	}

	lone_lisp_recalculate_heap_bounds(lone);

zero_pinned:
	lone_memory_zero(lone->heap.bits.pinned, lone_lisp_heap_bitmap_size(lone->heap.capacity));
}

void lone_lisp_garbage_collector(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	lone_lisp_mark_all_reachable_values(lone, machine);
	lone_lisp_kill_all_unmarked_values(lone);
	lone_lisp_compact_heap(lone, machine);
}
