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

#define LONE_ELF_HEADER_COMMON_READER(field, sign, bits)                                           \
struct lone_optional_##sign##bits                                                                  \
lone_elf_header_read_##field(struct lone_elf_header *header)                                       \
{                                                                                                  \
	return lone_elf_read_##sign##bits(header, &header->field);                                 \
}

#define LONE_ELF_HEADER_CLASSIFIED_READER(field, sign, bits)                                       \
struct lone_optional_##sign##bits                                                                  \
lone_elf_header_read_##field(struct lone_elf_header *header)                                       \
{                                                                                                  \
	return lone_elf_read_classified_##sign##bits(header,                                       \
			&header->as.elf32.field, &header->as.elf64.field);                         \
}

static struct lone_elf_value lone_elf_read_address_or_offset(struct lone_elf_header *header,
		void *address32, void *address64)
{
	struct lone_optional_u32 u32;
	struct lone_optional_u64 u64;
	struct lone_elf_value value = {
		.class = LONE_ELF_IDENT_CLASS_INVALID,
		.as.u64 = 0,
	};

	if (!header) { return value; }

	switch (header->ident[LONE_ELF_IDENT_INDEX_CLASS]) {
	case LONE_ELF_IDENT_CLASS_32BIT:
		u32 = lone_elf_read_u32(header, address32);
		if (u32.present) {
			value.class  = LONE_ELF_IDENT_CLASS_32BIT;
			value.as.u32 = u32.value;
		}
		break;
	case LONE_ELF_IDENT_CLASS_64BIT:
		u64 = lone_elf_read_u64(header, address64);
		if (u64.present) {
			value.class  = LONE_ELF_IDENT_CLASS_64BIT;
			value.as.u64 = u64.value;
		}
		break;
	default:
		break;
	}

	return value;
}

#define LONE_ELF_HEADER_ADDRESS_OR_OFFSET_READER(field)                                            \
struct lone_elf_value                                                                              \
lone_elf_header_read_##field(struct lone_elf_header *header)                                       \
{                                                                                                  \
	return lone_elf_read_address_or_offset(header,                                             \
			&header->as.elf32.field, &header->as.elf64.field);                         \
}

LONE_ELF_HEADER_COMMON_READER(type,    u, 16)
LONE_ELF_HEADER_COMMON_READER(machine, u, 16)
LONE_ELF_HEADER_COMMON_READER(version, u, 32)

LONE_ELF_HEADER_ADDRESS_OR_OFFSET_READER(entry_point)
LONE_ELF_HEADER_ADDRESS_OR_OFFSET_READER(segments_offset)
LONE_ELF_HEADER_ADDRESS_OR_OFFSET_READER(sections_offset)

LONE_ELF_HEADER_CLASSIFIED_READER(flags,               u, 32)
LONE_ELF_HEADER_CLASSIFIED_READER(header_size,         u, 16)
LONE_ELF_HEADER_CLASSIFIED_READER(segment_size,        u, 16)
LONE_ELF_HEADER_CLASSIFIED_READER(segment_count,       u, 16)
LONE_ELF_HEADER_CLASSIFIED_READER(section_size,        u, 16)
LONE_ELF_HEADER_CLASSIFIED_READER(section_count,       u, 16)
LONE_ELF_HEADER_CLASSIFIED_READER(section_names_index, u, 16)

#undef LONE_ELF_HEADER_ADDRESS_OR_OFFSET_READER
#undef LONE_ELF_HEADER_CLASSIFIED_READER
#undef LONE_ELF_HEADER_COMMON_READER

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    ELF header validation functions.                                    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

static bool is_within_u16(lone_u16 n, lone_u16 min, lone_u16 max)
{
	return n >= min && n <= max;
}

static bool is_within_u32(lone_u32 n, lone_u32 min, lone_u32 max)
{
	return n >= min && n <= max;
}

static bool ident_is_within_u16(struct lone_elf_header *header,
		enum lone_elf_ident_index index, lone_u16 min, lone_u16 max)
{
	return is_within_u16(header->ident[index], min, max);
}

bool lone_elf_header_ident_has_valid_magic_numbers(struct lone_elf_header *header)
{
	return header &&
	       lone_bytes_is_equal(lone_elf_header_read_ident_magic(header),
	                           LONE_ELF_IDENT_MAGIC_BYTES());
}

bool lone_elf_header_ident_has_zero_filled_padding(struct lone_elf_header *header)
{
	return header &&
	       lone_bytes_is_zero(lone_elf_header_read_ident_padding(header));
}

#define LONE_ELF_HEADER_IDENT_RANGE_CHECKER(name, constant)                                        \
bool lone_elf_header_ident_has_valid_##name(struct lone_elf_header *header)                        \
{                                                                                                  \
	return header &&                                                                           \
	       ident_is_within_u16(header,                                                         \
			LONE_ELF_IDENT_INDEX_##constant,                                           \
			LONE_ELF_RANGES_IDENT_##constant##_MIN,                                    \
			LONE_ELF_RANGES_IDENT_##constant##_MAX);                                   \
}

LONE_ELF_HEADER_IDENT_RANGE_CHECKER(class,         CLASS)
LONE_ELF_HEADER_IDENT_RANGE_CHECKER(data_encoding, DATA_ENCODING)
LONE_ELF_HEADER_IDENT_RANGE_CHECKER(version,       VERSION)
LONE_ELF_HEADER_IDENT_RANGE_CHECKER(os_abi,        OS_ABI)

#undef LONE_ELF_HEADER_IDENT_RANGE_CHECKER

bool lone_elf_header_ident_is_linux_os_abi(struct lone_elf_header *header)
{
	return header &&
	       header->ident[LONE_ELF_IDENT_INDEX_OS_ABI] == LONE_ELF_IDENT_OS_ABI_LINUX;
}

bool lone_elf_header_has_valid_ident(struct lone_elf_header *header)
{
	return header                                                  &&
	       lone_elf_header_ident_has_valid_magic_numbers(header)   &&
	       lone_elf_header_ident_has_valid_class(header)           &&
	       lone_elf_header_ident_has_valid_data_encoding(header)   &&
	       lone_elf_header_ident_has_valid_version(header)         &&
	       lone_elf_header_ident_has_valid_os_abi(header)          &&
	       lone_elf_header_ident_has_zero_filled_padding(header);
}

#define LONE_ELF_HEADER_RANGE_PREDICATE(name, predicate, constant)                                 \
bool lone_elf_header_##name##_is_##predicate(lone_u16 name)                                        \
{                                                                                                  \
	return is_within_u16(name,                                                                 \
			LONE_ELF_RANGES_##constant##_MIN,                                          \
			LONE_ELF_RANGES_##constant##_MAX);                                         \
}

LONE_ELF_HEADER_RANGE_PREDICATE(type, general, TYPE_GENERAL)
LONE_ELF_HEADER_RANGE_PREDICATE(type, os,      TYPE_OS)
LONE_ELF_HEADER_RANGE_PREDICATE(type, proc,    TYPE_PROC)

#undef LONE_ELF_HEADER_RANGE_PREDICATE

bool lone_elf_header_type_is_specific(lone_u16 type)
{
	return lone_elf_header_type_is_os(type) || lone_elf_header_type_is_proc(type);
}

bool lone_elf_header_has_valid_type(struct lone_elf_header *header)
{
	struct lone_optional_u16 type;

	if (!header) { return false; }

	type = lone_elf_header_read_type(header);

	if (!type.present) { return false; }

	return lone_elf_header_type_is_general(type.value) ||
	       lone_elf_header_type_is_os(type.value)      ||
	       lone_elf_header_type_is_proc(type.value);
}

bool lone_elf_header_machine_is_reserved(lone_u16 machine)
{
	return machine > LONE_ELF_RANGES_MACHINE_MAX ||
	       machine == 16                         ||
	       machine == 182                        ||
	       machine == 184                        ||
	       is_within_u16(machine,  11,  14)      ||
	       is_within_u16(machine,  24,  35)      ||
	       is_within_u16(machine, 121, 130)      ||
	       is_within_u16(machine, 145, 159)      ||
	       is_within_u16(machine, 145, 159)      ||
	       is_within_u16(machine, 225, 242);
}

bool lone_elf_header_has_valid_machine(struct lone_elf_header *header)
{
	struct lone_optional_u16 machine;

	if (!header) { return false; }

	machine = lone_elf_header_read_machine(header);

	if (!machine.present) { return false; }

	return is_within_u16(machine.value,
			LONE_ELF_RANGES_MACHINE_MIN,
			LONE_ELF_RANGES_MACHINE_MAX) &&
		!lone_elf_header_machine_is_reserved(machine.value);
}

bool lone_elf_header_has_valid_version(struct lone_elf_header *header)
{
	struct lone_optional_u32 version;

	if (!header) { return false; }

	version = lone_elf_header_read_version(header);

	if (!version.present) { return false; }

	return is_within_u32(version.value,
			LONE_ELF_RANGES_VERSION_MIN,
			LONE_ELF_RANGES_VERSION_MAX);
}

bool lone_elf_header_has_valid_header_size(struct lone_elf_header *header)
{
	struct lone_optional_u16 header_size;

	if (!header) { return false; }

	header_size = lone_elf_header_read_header_size(header);

	if (!header_size.present) { return false; }

	switch (header->ident[LONE_ELF_IDENT_INDEX_CLASS]) {
	case LONE_ELF_IDENT_CLASS_32BIT:
		return header_size.value == sizeof(Elf32_Ehdr);
	case LONE_ELF_IDENT_CLASS_64BIT:
		return header_size.value == sizeof(Elf64_Ehdr);
	case LONE_ELF_IDENT_CLASS_INVALID:
	default:
		return false;
	}
}

bool lone_elf_header_is_valid(struct lone_elf_header *header)
{
	return lone_elf_header_has_valid_ident(header)          &&
	       lone_elf_header_has_valid_type(header)           &&
	       lone_elf_header_has_valid_machine(header)        &&
	       lone_elf_header_has_valid_version(header)        &&
	       lone_elf_header_has_valid_header_size(header);
}
