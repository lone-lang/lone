/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/elf.h>

static struct lone_bytes lone_elf_header_read_ident_bytes(struct lone_elf_header *header,
		enum lone_elf_ident_index index, enum lone_elf_sizes_ident size)
{
	if (!header) {
		return LONE_BYTES_VALUE_NULL();
	}

	return LONE_BYTES_VALUE(size, &header->ident[index]);
}

#define LONE_ELF_HEADER_IDENT_BYTES_READER(name, constant)                                         \
struct lone_bytes lone_elf_header_read_ident_##name(struct lone_elf_header *header)                \
{                                                                                                  \
	enum lone_elf_ident_index index = LONE_ELF_IDENT_INDEX_##constant;                         \
	enum lone_elf_sizes_ident size  = LONE_ELF_SIZES_IDENT_##constant;                         \
	                                                                                           \
	return lone_elf_header_read_ident_bytes(header, index, size);                              \
}

LONE_ELF_HEADER_IDENT_BYTES_READER(data,    DATA)
LONE_ELF_HEADER_IDENT_BYTES_READER(padding, PADDING)
LONE_ELF_HEADER_IDENT_BYTES_READER(magic,   MAGIC)

#undef LONE_ELF_HEADER_IDENT_BYTES_READER
