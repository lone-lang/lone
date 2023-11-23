/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_AUXILIARY_VECTOR_HEADER
#define LONE_AUXILIARY_VECTOR_HEADER

#include <lone/definitions.h>
#include <lone/types.h>

struct lone_auxiliary_value lone_auxiliary_vector_value(struct lone_auxiliary_vector *values, long type);
size_t lone_auxiliary_vector_page_size(struct lone_auxiliary_vector *values);
struct lone_bytes lone_auxiliary_vector_random(struct lone_auxiliary_vector *values);
struct lone_elf_segments lone_auxiliary_vector_elf_segments(struct lone_auxiliary_vector *values);
lone_elf_segment *lone_auxiliary_vector_embedded_segment(struct lone_auxiliary_vector *values);
struct lone_bytes lone_auxiliary_vector_embedded_bytes(struct lone_auxiliary_vector *values);

#endif /* LONE_AUXILIARY_VECTOR_HEADER */
