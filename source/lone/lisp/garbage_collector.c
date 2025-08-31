/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/garbage_collector.h>
#include <lone/lisp/heap.h>

#include <lone/memory/allocator.h>

#include <lone/architecture/garbage_collector.c>

static void lone_lisp_mark_heap_value(struct lone_lisp_heap_value *);

static void lone_lisp_mark_value(struct lone_lisp_value value)
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

	lone_lisp_mark_heap_value(lone_lisp_heap_value_of(value));
}

static void lone_lisp_mark_heap_value(struct lone_lisp_heap_value *value)
{
	if (!value || !value->live || value->marked) { return; }

	value->marked = true;

	switch (value->type) {
	case LONE_LISP_TYPE_MODULE:
		lone_lisp_mark_value(value->as.module.name);
		lone_lisp_mark_value(value->as.module.environment);
		lone_lisp_mark_value(value->as.module.exports);
		break;
	case LONE_LISP_TYPE_FUNCTION:
		lone_lisp_mark_value(value->as.function.arguments);
		lone_lisp_mark_value(value->as.function.code);
		lone_lisp_mark_value(value->as.function.environment);
		break;
	case LONE_LISP_TYPE_PRIMITIVE:
		lone_lisp_mark_value(value->as.primitive.name);
		lone_lisp_mark_value(value->as.primitive.closure);
		break;
	case LONE_LISP_TYPE_LIST:
		lone_lisp_mark_value(value->as.list.first);
		lone_lisp_mark_value(value->as.list.rest);
		break;
	case LONE_LISP_TYPE_VECTOR:
		for (size_t i = 0; i < value->as.vector.count; ++i) {
			lone_lisp_mark_value(value->as.vector.values[i]);
		}
		break;
	case LONE_LISP_TYPE_TABLE:
		lone_lisp_mark_value(value->as.table.prototype);
		for (size_t i = 0; i < value->as.table.count; ++i) {
			lone_lisp_mark_value(value->as.table.entries[i].key);
			lone_lisp_mark_value(value->as.table.entries[i].value);
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
	lone_lisp_mark_value(lone->symbol_table);
	lone_lisp_mark_value(lone->modules.loaded);
	lone_lisp_mark_value(lone->modules.embedded);
	lone_lisp_mark_value(lone->modules.null);
	lone_lisp_mark_value(lone->modules.top_level_environment);
	lone_lisp_mark_value(lone->modules.path);
}

static bool lone_points_within_range(void *pointer, void *start, void *end)
{
	return start <= pointer && pointer < end;
}

static bool lone_lisp_points_to_heap(struct lone_lisp *lone, void *pointer)
{
	struct lone_lisp_heap *heap;

	for (heap = lone->heaps; heap; heap = heap->next) {
		if (lone_points_within_range(pointer, heap->values, heap->values + LONE_LISP_HEAP_VALUE_COUNT)) {
			return true;
		}
	}

	return false;
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
			lone_lisp_mark_heap_value(*pointer);
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

static void lone_lisp_mark_lisp_stack_roots(struct lone_lisp *lone)
{
	lone_lisp_mark_stack_roots(
		lone,
		lone->machine.stack.base,
		lone->machine.stack.top
	);
}

static void lone_lisp_mark_all_reachable_values(struct lone_lisp *lone)
{
	lone_registers registers;                     /* stack space for registers */
	lone_save_registers(registers);               /* spill registers on stack */

	lone_lisp_mark_known_roots(lone);           /* precise */
	lone_lisp_mark_lisp_stack_roots(lone);      /* precise */
	lone_lisp_mark_native_stack_roots(lone);    /* conservative */
}

static void lone_lisp_kill_all_unmarked_values(struct lone_lisp *lone)
{
	struct lone_lisp_heap_value *value;
	struct lone_lisp_heap *heap;
	size_t i;

	for (heap = lone->heaps; heap; heap = heap->next) {
		for (i = 0; i < LONE_LISP_HEAP_VALUE_COUNT; ++i) {
			value = &heap->values[i];

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
}

void lone_lisp_garbage_collector(struct lone_lisp *lone)
{
	lone_lisp_mark_all_reachable_values(lone);
	lone_lisp_kill_all_unmarked_values(lone);
	lone_lisp_deallocate_dead_heaps(lone);
}
