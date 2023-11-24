#include <lone/segment.h>

struct lone_bytes lone_segment_bytes(lone_elf_segment *segment)
{
	if (!segment) {
		return (struct lone_bytes) {
			.count = 0,
			.pointer = 0
		};
	}

	return (struct lone_bytes) {
		.count = segment->p_memsz,
		.pointer = (unsigned char *) segment->p_vaddr
	};
}
