#include <lone/memory/layout.h>
#include <lone/memory/array.h>
#include <lone/memory/allocator.h>

#include <lone/value.h>
#include <lone/value/integer.h>
#include <lone/value/pointer.h>

static size_t lone_memory_layout_element_size_homogeneous(struct lone_memory_layout layout)
{
	switch (layout.register_value_type) {
	case LONE_NIL:
		return 0;
	case LONE_INTEGER:
		return LONE_SIZE_OF_MEMBER(struct lone_value, as.integer);
	case LONE_POINTER:
		return LONE_SIZE_OF_MEMBER(struct lone_value, as.pointer);
	case LONE_HEAP_VALUE:
		return LONE_SIZE_OF_MEMBER(struct lone_value, as.heap_value);
	}
}

static size_t lone_memory_layout_element_size_heterogeneous(void)
{
	return sizeof(struct lone_value);
}

static size_t lone_memory_layout_element_size(struct lone_memory_layout layout)
{
	switch (layout.state) {
	case LONE_MEMORY_LAYOUT_STATE_EMPTY:
		return 0;
	case LONE_MEMORY_LAYOUT_STATE_HOMOGENEOUS:
		return lone_memory_layout_element_size_homogeneous(layout);
	case LONE_MEMORY_LAYOUT_STATE_HETEROGENEOUS:
		return lone_memory_layout_element_size_heterogeneous();
	}
}

static size_t lone_memory_layout_element_offset(struct lone_memory_layout layout, size_t index)
{
	return lone_memory_array_size_in_bytes(index, lone_memory_layout_element_size(layout));
}

static void * lone_memory_layout_element_at(struct lone_memory_layout layout, size_t index)
{
	return ((unsigned char *) layout.bytes.pointer) + lone_memory_layout_element_offset(layout, index);
}

static struct lone_value lone_memory_layout_get_nil(struct lone_memory_layout layout, size_t index)
{
	return lone_nil();
}

static void lone_memory_layout_set_nil(struct lone_memory_layout layout, size_t index, struct lone_value value)
{
	/* no need to actually store the nil */
}

static struct lone_value lone_memory_layout_get_integer(struct lone_memory_layout layout, size_t index)
{
	lone_integer *integer = lone_memory_layout_element_at(layout, index);
	return lone_integer_create(*integer);
}

static void lone_memory_layout_set_integer(struct lone_memory_layout layout, size_t index, struct lone_value value)
{
	lone_integer *integer = lone_memory_layout_element_at(layout, index);
	*integer = value.as.integer;
}

static struct lone_value lone_memory_layout_get_pointer(struct lone_memory_layout layout, size_t index)
{
	union lone_pointer *pointer = lone_memory_layout_element_at(layout, index);
	return lone_pointer_create(pointer->to_void, layout.pointer_type);
}

static void lone_memory_layout_set_pointer(struct lone_memory_layout layout, size_t index, struct lone_value value)
{
	union lone_pointer *pointer = lone_memory_layout_element_at(layout, index);
	pointer->to_void = value.as.pointer.to_void;
}

static struct lone_value lone_memory_layout_get_heap_value(struct lone_memory_layout layout, size_t index)
{
	struct lone_heap_value **actual = lone_memory_layout_element_at(layout, index);
	return lone_value_from_heap_value(*actual);
}

static void lone_memory_layout_set_heap_value(struct lone_memory_layout layout, size_t index, struct lone_value value)
{
	struct lone_heap_value **actual = lone_memory_layout_element_at(layout, index);
	*actual = value.as.heap_value;
}

static struct lone_value lone_memory_layout_get_value(struct lone_memory_layout layout, size_t index)
{
	struct lone_value *value = lone_memory_layout_element_at(layout, index);
	return *value;
}

static void lone_memory_layout_set_value(struct lone_memory_layout layout, size_t index, struct lone_value value)
{
	struct lone_value *element = lone_memory_layout_element_at(layout, index);
	*element = value;
}

static struct lone_value lone_memory_layout_get_homogeneous(struct lone_memory_layout layout, size_t index)
{
	switch (layout.register_value_type) {
	case LONE_NIL:
		return lone_memory_layout_get_nil(layout, index);
	case LONE_INTEGER:
		return lone_memory_layout_get_integer(layout, index);
	case LONE_POINTER:
		return lone_memory_layout_get_pointer(layout, index);
	case LONE_HEAP_VALUE:
		return lone_memory_layout_get_heap_value(layout, index);
	}
}

static struct lone_value lone_memory_layout_get_heterogeneous(struct lone_memory_layout layout, size_t index)
{
	return lone_memory_layout_get_value(layout, index);
}

static void lone_memory_layout_set_homogeneous(struct lone_memory_layout layout, size_t index, struct lone_value value)
{
	switch (layout.register_value_type) {
	case LONE_NIL:
		lone_memory_layout_set_nil(layout, index, value);
		break;
	case LONE_INTEGER:
		lone_memory_layout_set_integer(layout, index, value);
		break;
	case LONE_POINTER:
		lone_memory_layout_set_pointer(layout, index, value);
		break;
	case LONE_HEAP_VALUE:
		lone_memory_layout_set_heap_value(layout, index, value);
		break;
	}
}

static void lone_memory_layout_set_heterogeneous(struct lone_memory_layout layout, size_t index, struct lone_value value)
{
	lone_memory_layout_set_value(layout, index, value);
}

static void lone_memory_layout_empty(struct lone_memory_layout *layout)
{
	layout->state = LONE_MEMORY_LAYOUT_STATE_EMPTY;
}

static void lone_memory_layout_homogeneize(struct lone_memory_layout *layout, struct lone_value value)
{
	layout->state = LONE_MEMORY_LAYOUT_STATE_HOMOGENEOUS;

	layout->register_value_type = value.type;

	if (lone_is_pointer(value)) {
		layout->pointer_type = value.pointer_type;
	}

	if (lone_is_heap_value(value)) {
		layout->heap_value_type = value.as.heap_value->type;
	}
}

static void lone_memory_layout_heterogeneize(struct lone_lisp *lone, struct lone_memory_layout *layout)
{
	struct lone_memory_layout new;
	size_t i;

	new = lone_memory_layout_create(lone, layout->bytes.count / lone_memory_layout_element_size(*layout));
	new.state = LONE_MEMORY_LAYOUT_STATE_HETEROGENEOUS;
	new.count = layout->count;

	for (i = 0; i < new.count; ++i) {
		lone_memory_layout_set_heterogeneous(new, i, lone_memory_layout_get_homogeneous(*layout, i));
	}

	lone_deallocate(lone, layout->bytes.pointer);

	*layout = new;
}

static bool lone_memory_layout_must_be_heterogeneized(struct lone_memory_layout layout, struct lone_value value)
{
	bool still_homogeneous = layout.register_value_type == value.type;

	if (still_homogeneous && lone_is_pointer(value)) {
		still_homogeneous = still_homogeneous && layout.pointer_type == value.pointer_type;
	}

	if (still_homogeneous && lone_is_heap_value(value)) {
		still_homogeneous = still_homogeneous && layout.heap_value_type == value.as.heap_value->type;
	}

	return !still_homogeneous;
}

static size_t lone_memory_layout_end(struct lone_memory_layout layout)
{
	return lone_memory_layout_element_offset(layout, layout.count);
}

static bool lone_memory_layout_has_overrun(struct lone_memory_layout layout)
{
	return lone_memory_layout_end(layout) > layout.bytes.count;
}

struct lone_value lone_memory_layout_get(struct lone_memory_layout *layout, size_t index)
{
	if (layout->count == 0) { lone_memory_layout_empty(layout); }

	switch (layout->state) {
	case LONE_MEMORY_LAYOUT_STATE_EMPTY:
		return lone_nil();
	case LONE_MEMORY_LAYOUT_STATE_HOMOGENEOUS:
		return lone_memory_layout_get_homogeneous(*layout, index);
	case LONE_MEMORY_LAYOUT_STATE_HETEROGENEOUS:
		return lone_memory_layout_get_heterogeneous(*layout, index);
	}
}

void lone_memory_layout_set(struct lone_lisp *lone, struct lone_memory_layout *layout, size_t index, struct lone_value value)
{
	if (layout->count == 0) { lone_memory_layout_empty(layout); }
	if (index + 1 > layout->count) { layout->count = index + 1; }

	switch (layout->state) {
	case LONE_MEMORY_LAYOUT_STATE_EMPTY:
		lone_memory_layout_homogeneize(layout, value);
		lone_memory_layout_set_homogeneous(*layout, index, value);
		break;
	case LONE_MEMORY_LAYOUT_STATE_HOMOGENEOUS:
		if (lone_memory_layout_must_be_heterogeneized(*layout, value)) {
			lone_memory_layout_heterogeneize(lone, layout);
			lone_memory_layout_set_heterogeneous(*layout, index, value);
		} else {
			lone_memory_layout_set_homogeneous(*layout, index, value);
		}
		break;
	case LONE_MEMORY_LAYOUT_STATE_HETEROGENEOUS:
		lone_memory_layout_set_heterogeneous(*layout, index, value);
		break;
	}
}

bool lone_memory_layout_is_bounded(struct lone_memory_layout layout, size_t index)
{
	return lone_memory_layout_element_offset(layout, index + 1) <= layout.bytes.count;
}

struct lone_memory_layout lone_memory_layout_create(struct lone_lisp *lone, size_t capacity)
{
	size_t size = lone_memory_layout_element_size_heterogeneous();
	return (struct lone_memory_layout) {
		.state = LONE_MEMORY_LAYOUT_STATE_EMPTY,
		.count = 0,
		.bytes = (struct lone_bytes) {
			.count = lone_memory_array_size_in_bytes(capacity, size),
			.pointer = lone_memory_array(lone, 0, capacity, size)
		}
	};
}

void lone_memory_layout_resize(struct lone_lisp *lone, struct lone_memory_layout *layout, size_t capacity)
{
	layout->bytes.pointer = lone_memory_array(lone, layout->bytes.pointer, capacity, lone_memory_layout_element_size(*layout));
	if (lone_memory_layout_has_overrun(*layout)) {
		/* array shrank, truncate count */
		layout->count = capacity;
	}
}
