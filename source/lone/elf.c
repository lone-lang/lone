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

#define LONE_ELF_MULTIBYTE_VALUE_READER(sign, bits)                                                \
static struct lone_optional_##sign##bits                                                           \
lone_elf_read_##sign##bits(struct lone_elf_header *header, void *address)                          \
{                                                                                                  \
	lone_##sign##bits value;                                                                   \
	                                                                                           \
	if (!header) {                                                                             \
		return LONE_OPTIONAL_ABSENT_VALUE(sign##bits);                                     \
	}                                                                                          \
	                                                                                           \
	switch (header->ident[LONE_ELF_IDENT_INDEX_DATA_ENCODING]) {                               \
	case LONE_ELF_IDENT_DATA_ENCODING_LITTLE_ENDIAN:                                           \
		value = lone_##sign##bits##_read_le(address);                                      \
		break;                                                                             \
	case LONE_ELF_IDENT_DATA_ENCODING_BIG_ENDIAN:                                              \
		value = lone_##sign##bits##_read_be(address);                                      \
		break;                                                                             \
	default:                                                                                   \
		return LONE_OPTIONAL_ABSENT_VALUE(sign##bits);                                     \
	}                                                                                          \
	                                                                                           \
	return LONE_OPTIONAL_PRESENT_VALUE(sign##bits, value);                                     \
}

LONE_ELF_MULTIBYTE_VALUE_READER(u, 16)
LONE_ELF_MULTIBYTE_VALUE_READER(u, 32)
LONE_ELF_MULTIBYTE_VALUE_READER(u, 64)
LONE_ELF_MULTIBYTE_VALUE_READER(s, 32)
LONE_ELF_MULTIBYTE_VALUE_READER(s, 64)

#undef LONE_ELF_MULTIBYTE_VALUE_READER

#define LONE_ELF_MULTIBYTE_MULTICLASS_VALUE_READER(sign, bits)                                     \
static struct lone_optional_##sign##bits                                                           \
lone_elf_read_classified_##sign##bits(struct lone_elf_header *header,                              \
		void *address32, void *address64)                                                  \
{                                                                                                  \
	if (!header) {                                                                             \
		return LONE_OPTIONAL_ABSENT_VALUE(sign##bits);                                     \
	}                                                                                          \
	                                                                                           \
	switch (header->ident[LONE_ELF_IDENT_INDEX_CLASS]) {                                       \
	case LONE_ELF_IDENT_CLASS_32BIT:                                                           \
		return lone_elf_read_##sign##bits(header, address32);                              \
	case LONE_ELF_IDENT_CLASS_64BIT:                                                           \
		return lone_elf_read_##sign##bits(header, address64);                              \
	default:                                                                                   \
		return LONE_OPTIONAL_ABSENT_VALUE(sign##bits);                                     \
	}                                                                                          \
}

LONE_ELF_MULTIBYTE_MULTICLASS_VALUE_READER(u, 16)
LONE_ELF_MULTIBYTE_MULTICLASS_VALUE_READER(u, 32)
LONE_ELF_MULTIBYTE_MULTICLASS_VALUE_READER(u, 64)
LONE_ELF_MULTIBYTE_MULTICLASS_VALUE_READER(s, 32)
LONE_ELF_MULTIBYTE_MULTICLASS_VALUE_READER(s, 64)

#undef LONE_ELF_MULTIBYTE_MULTICLASS_VALUE_READER
