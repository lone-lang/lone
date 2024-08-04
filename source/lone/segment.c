/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/segment.h>

struct lone_bytes lone_segment_bytes(lone_elf_native_segment *segment)
{
	if (!segment) {
		return LONE_BYTES_VALUE_NULL();
	}

	return LONE_BYTES_VALUE(segment->p_memsz, segment->p_vaddr);
}
