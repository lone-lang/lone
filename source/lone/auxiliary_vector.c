/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/auxiliary_vector.h>

struct lone_auxiliary_value lone_auxiliary_vector_value(struct lone_auxiliary_vector *auxiliary, unsigned long type)
{
	for (/* auxiliary */; auxiliary->type != AT_NULL; ++auxiliary) {
		if (auxiliary->type == type) {
			return auxiliary->value;
		}
	}

	return (struct lone_auxiliary_value) { .as.unsigned_integer = 0 };
}

size_t lone_auxiliary_vector_page_size(struct lone_auxiliary_vector *auxiliary)
{
	return lone_auxiliary_vector_value(auxiliary, AT_PAGESZ).as.unsigned_integer;
}

struct lone_bytes lone_auxiliary_vector_random(struct lone_auxiliary_vector *auxiliary)
{
	struct lone_bytes random = { 0, 0 };

	random.pointer = lone_auxiliary_vector_value(auxiliary, AT_RANDOM).as.pointer;
	random.count = 16;

	return random;
}

struct lone_elf_segments lone_auxiliary_vector_elf_segments(struct lone_auxiliary_vector *auxiliary)
{
	return (struct lone_elf_segments) {
		.entry_size  = lone_auxiliary_vector_value(auxiliary, AT_PHENT).as.unsigned_integer,
		.entry_count = lone_auxiliary_vector_value(auxiliary, AT_PHNUM).as.unsigned_integer,
		.segments    = lone_auxiliary_vector_value(auxiliary, AT_PHDR).as.pointer
	};
}

lone_elf_segment *lone_auxiliary_vector_embedded_segment(struct lone_auxiliary_vector *values)
{
	struct lone_elf_segments table;
	size_t i;

	table = lone_auxiliary_vector_elf_segments(values);

	for (i = 0; i < table.entry_count; ++i) {
		lone_elf_segment *entry = &table.segments[i];

		if (entry->p_type == PT_LONE) {
			return entry;
		}
	}

	return 0;
}
