/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_SEGMENT_HEADER
#define LONE_SEGMENT_HEADER

#include <lone/types.h>
#include <lone/elf.h>

struct lone_bytes lone_segment_bytes(lone_elf_native_segment *segment);

#endif /* LONE_SEGMENT_HEADER */
