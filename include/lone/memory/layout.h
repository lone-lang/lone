#ifndef LONE_MEMORY_LAYOUT_HEADER
#define LONE_MEMORY_LAYOUT_HEADER

#include <lone/types.h>

struct lone_memory_layout lone_memory_layout_create(struct lone_lisp *lone, size_t capacity);
void lone_memory_layout_resize(struct lone_lisp *lone, struct lone_memory_layout *layout, size_t capacity);
bool lone_memory_layout_is_bounded(struct lone_memory_layout layout, size_t index);

struct lone_value lone_memory_layout_get(struct lone_memory_layout *layout, size_t index);
void lone_memory_layout_set(struct lone_lisp *lone, struct lone_memory_layout *layout, size_t index, struct lone_value value);

#endif /* LONE_MEMORY_LAYOUT_HEADER */
