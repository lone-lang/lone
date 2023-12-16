#include <lone/memory/array.h>
#include <lone/memory/allocator.h>
#include <lone/linux.h>

#define MULTIPLICATION_OVERFLOWED(result, x, y) __builtin_mul_overflow((x), (y), (result))

size_t __attribute__((const)) lone_memory_array_size_in_bytes(size_t element_count, size_t element_size)
{
	size_t size_in_bytes;

	/* size_in_bytes = element_count * element_size; overflowed? */
	if (MULTIPLICATION_OVERFLOWED(&size_in_bytes, element_count, element_size)) {
		linux_exit(-1); /* integer overflow */
	}

	return size_in_bytes;
}

bool lone_memory_array_is_bounded(size_t element_index, size_t element_capacity, size_t element_size)
{
	size_t end_of_indexed_element = lone_memory_array_size_in_bytes(element_index + 1, element_size);
	size_t end_of_buffer          = lone_memory_array_size_in_bytes(element_capacity,  element_size);

	if (end_of_indexed_element > end_of_buffer) {
		return false; /* buffer overrun */
	} else {
		return true;
	}
}

void * lone_memory_array(struct lone_lisp *lone, void *buffer_or_null, size_t element_count, size_t element_size)
{
	return lone_reallocate(lone, buffer_or_null, lone_memory_array_size_in_bytes(element_count, element_size));
}
