#include <lone/memory.h>

static size_t __attribute__((const)) lone_next_power_of_2(size_t n)
{
	size_t next = 1;
	while (next < n) { next *= 2; }
	return next;
}

static size_t __attribute__((const)) lone_next_power_of_2_multiple(size_t n, size_t m)
{
	m = lone_next_power_of_2(m);
	return (n + m - 1) & (~(m - 1));
}

size_t __attribute__((const)) lone_align(size_t size, size_t alignment)
{
	return lone_next_power_of_2_multiple(size, alignment);
}

