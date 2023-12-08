#ifndef LONE_SEGMENT_HEADER
#define LONE_SEGMENT_HEADER

#include <lone/types.h>

struct lone_bytes lone_segment_bytes(lone_elf_segment *segment);
struct lone_value lone_segment_read_descriptor(struct lone_lisp *lone, lone_elf_segment *segment);

#endif /* LONE_SEGMENT_HEADER */
