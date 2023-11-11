#include <lone/definitions.h>
#include <lone/memory.h>
#include <lone/memory/heap.h>

#include <lone/struct/lisp.h>
#include <lone/struct/memory.h>

void lone_memory_initialize(struct lone_lisp *lone, struct lone_bytes memory, size_t heap_size, void *stack)
{
	lone->memory.stack = stack;

	lone->memory.general = (struct lone_memory *) __builtin_assume_aligned(memory.pointer, LONE_ALIGNMENT);
	lone->memory.general->prev = lone->memory.general->next = 0;
	lone->memory.general->free = 1;
	lone->memory.general->size = memory.count - sizeof(struct lone_memory);

	lone_heap_initialize(lone, heap_size);
}
