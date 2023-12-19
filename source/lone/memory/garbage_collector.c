/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/memory/garbage_collector.h>
#include <lone/memory/allocator.h>
#include <lone/memory/heap.h>
#include <lone/memory/layout.h>

#include <lone/architecture/garbage_collector.c>

static void lone_mark_heap_value(struct lone_heap_value *);

static void lone_mark_value(struct lone_value value)
{
	struct lone_heap_value *actual;

	switch (value.type) {
	case LONE_NIL:
	case LONE_INTEGER:
	case LONE_POINTER:
		/* value types need not be marked */
		return;
	case LONE_HEAP_VALUE:
		break;
	}

	actual = value.as.heap_value;

	lone_mark_heap_value(actual);
}

static void lone_mark_heap_value(struct lone_heap_value *value)
{
	if (!value || !value->live || value->marked) { return; }

	value->marked = true;

	switch (value->type) {
	case LONE_MODULE:
		lone_mark_value(value->as.module.name);
		lone_mark_value(value->as.module.environment);
		lone_mark_value(value->as.module.exports);
		break;
	case LONE_FUNCTION:
		lone_mark_value(value->as.function.arguments);
		lone_mark_value(value->as.function.code);
		lone_mark_value(value->as.function.environment);
		break;
	case LONE_PRIMITIVE:
		lone_mark_value(value->as.primitive.name);
		lone_mark_value(value->as.primitive.closure);
		break;
	case LONE_LIST:
		lone_mark_value(value->as.list.first);
		lone_mark_value(value->as.list.rest);
		break;
	case LONE_VECTOR:
		for (size_t i = 0; i < value->as.vector.values.count; ++i) {
			lone_mark_value(lone_memory_layout_get(&value->as.vector.values, i));
		}
		break;
	case LONE_TABLE:
		lone_mark_value(value->as.table.prototype);
		for (size_t i = 0; i < value->as.table.count; ++i) {
			lone_mark_value(lone_memory_layout_get(&value->as.table.entries.keys, i));
			lone_mark_value(lone_memory_layout_get(&value->as.table.entries.values, i));
		}
		break;
	case LONE_SYMBOL:
	case LONE_TEXT:
	case LONE_BYTES:
		/* these types do not contain any other values to mark */
		break;
	}
}

static void lone_mark_known_roots(struct lone_lisp *lone)
{
	lone_mark_value(lone->symbol_table);
	lone_mark_value(lone->constants.truth);
	lone_mark_value(lone->modules.loaded);
	lone_mark_value(lone->modules.embedded);
	lone_mark_value(lone->modules.null);
	lone_mark_value(lone->modules.top_level_environment);
	lone_mark_value(lone->modules.path);
}

static bool lone_points_within_range(void *pointer, void *start, void *end)
{
	return start <= pointer && pointer < end;
}

static bool lone_points_to_general_memory(struct lone_lisp *lone, void *pointer)
{
	struct lone_memory *general = lone->memory.general;
	return lone_points_within_range(pointer, general->pointer, general->pointer + general->size);
}

static bool lone_points_to_heap(struct lone_lisp *lone, void *pointer)
{
	struct lone_heap *heap;

	if (!lone_points_to_general_memory(lone, pointer)) { return false; }

	for (heap = lone->memory.heaps; heap; heap = heap->next) {
		if (lone_points_within_range(pointer, heap->values, heap->values + LONE_MEMORY_HEAP_VALUE_COUNT)) { return true; }
	}

	return false;
}

static void lone_find_and_mark_stack_roots(struct lone_lisp *lone)
{
	void *bottom = lone->memory.stack, *top = __builtin_frame_address(0), *tmp;
	void **pointer;

	if (top < bottom) {
		tmp = bottom;
		bottom = top;
		top = tmp;
	}

	pointer = bottom;

	while (pointer++ < top) {
		if (lone_points_to_heap(lone, *pointer)) {
			lone_mark_heap_value(*pointer);
		}
	}
}

static void lone_mark_all_reachable_values(struct lone_lisp *lone)
{
	lone_registers registers;                /* stack space for registers */
	lone_save_registers(registers);          /* spill registers on stack */

	lone_mark_known_roots(lone);             /* precise */
	lone_find_and_mark_stack_roots(lone);    /* conservative */
}

static void lone_kill_all_unmarked_values(struct lone_lisp *lone)
{
	struct lone_heap_value *value;
	struct lone_heap *heap;
	size_t i;

	for (heap = lone->memory.heaps; heap; heap = heap->next) {
		for (i = 0; i < LONE_MEMORY_HEAP_VALUE_COUNT; ++i) {
			value = &heap->values[i];

			if (!value->live) { continue; }

			if (!value->marked) {
				switch (value->type) {
				case LONE_BYTES:
				case LONE_TEXT:
				case LONE_SYMBOL:
					if (value->should_deallocate_bytes) {
						lone_deallocate(lone, value->as.bytes.pointer);
					}
					break;
				case LONE_VECTOR:
					lone_deallocate(lone, value->as.vector.values.bytes.pointer);
					break;
				case LONE_TABLE:
					lone_deallocate(lone, value->as.table.indexes);
					lone_deallocate(lone, value->as.table.entries.keys.bytes.pointer);
					lone_deallocate(lone, value->as.table.entries.values.bytes.pointer);
					break;
				case LONE_MODULE:
				case LONE_FUNCTION:
				case LONE_PRIMITIVE:
				case LONE_LIST:
					/* these types do not own any additional memory */
					break;
				}

				value->live = false;
			}

			value->marked = false;
		}
	}
}

void lone_garbage_collector(struct lone_lisp *lone)
{
	lone_mark_all_reachable_values(lone);
	lone_kill_all_unmarked_values(lone);
	lone_deallocate_dead_heaps(lone);
}
