/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/types.h>
#include <lone/linux.h>
#include <lone/memory/functions.h>

bool lone_bytes_equals(struct lone_bytes x, struct lone_bytes y)
{
	if (x.count != y.count) return false;
	return lone_memory_compare(x.pointer, y.pointer, x.count) == 0;
}

bool lone_bytes_equals_c_string(struct lone_bytes bytes, char *c_string)
{
	struct lone_bytes c_string_bytes = { lone_c_string_length(c_string), (unsigned char *) c_string };
	return lone_bytes_equals(bytes, c_string_bytes);
}

bool lone_bytes_contains_offset(struct lone_bytes bytes, lone_size offset)
{
	return offset >= 0 && offset < bytes.count;
}

bool lone_bytes_contains_block(struct lone_bytes bytes, lone_size offset, lone_size size)
{
	return offset >= 0 && (offset + size) <= bytes.count;
}

bool lone_bytes_contains_address(struct lone_bytes bytes, void *pointer)
{
	void *start = bytes.pointer, *end = bytes.pointer + bytes.count;
	return pointer >= start && pointer < end;
}

#define LONE_READER(type) \
lone_##type lone_##type##_read(void *address) \
{ \
	lone_##type *type = address; \
	return *type; \
}

#define LONE_WRITER(type) \
void lone_##type##_write(void *address, lone_##type value) \
{ \
	lone_##type *type = address; \
	*type = value; \
}

LONE_READER(u8)
LONE_READER(s8)
LONE_READER(u16)
LONE_READER(s16)
LONE_READER(u32)
LONE_READER(s32)
LONE_READER(u64)
LONE_READER(s64)

LONE_WRITER(u8)
LONE_WRITER(s8)
LONE_WRITER(u16)
LONE_WRITER(s16)
LONE_WRITER(u32)
LONE_WRITER(s32)
LONE_WRITER(u64)
LONE_WRITER(s64)

#undef LONE_READER
#undef LONE_WRITER

#define LONE_BYTES_READER(type) \
lone_##type lone_bytes_read_##type(struct lone_bytes bytes, lone_size offset) \
{ \
	return lone_##type##_read(bytes.pointer + offset); \
}

#define LONE_BYTES_WRITER(type) \
void lone_bytes_write_##type(struct lone_bytes bytes, lone_size offset, lone_##type type) \
{ \
	lone_##type##_write(bytes.pointer + offset, type); \
}

LONE_BYTES_READER(u8)
LONE_BYTES_READER(s8)
LONE_BYTES_READER(u16)
LONE_BYTES_READER(s16)
LONE_BYTES_READER(u32)
LONE_BYTES_READER(s32)
LONE_BYTES_READER(u64)
LONE_BYTES_READER(s64)

LONE_BYTES_WRITER(u8)
LONE_BYTES_WRITER(s8)
LONE_BYTES_WRITER(u16)
LONE_BYTES_WRITER(s16)
LONE_BYTES_WRITER(u32)
LONE_BYTES_WRITER(s32)
LONE_BYTES_WRITER(u64)
LONE_BYTES_WRITER(s64)

#undef LONE_BYTES_READER
#undef LONE_BYTES_WRITER

#define LONE_BYTES_CHECKED_READER(type) \
bool lone_bytes_checked_read_##type(struct lone_bytes bytes, lone_size offset, lone_##type *type) \
{ \
	if (!lone_bytes_contains_block(bytes, offset, sizeof(*type))) { \
		return false; \
	} else { \
		*type = lone_bytes_read_##type(bytes, offset); \
		return true; \
	} \
}

#define LONE_BYTES_CHECKED_WRITER(type) \
bool lone_bytes_checked_write_##type(struct lone_bytes bytes, lone_size offset, lone_##type type) \
{ \
	if (!lone_bytes_contains_block(bytes, offset, sizeof(type))) { \
		return false; \
	} else { \
		lone_bytes_write_##type(bytes, offset, type); \
		return true; \
	} \
}

LONE_BYTES_CHECKED_READER(u8)
LONE_BYTES_CHECKED_READER(s8)
LONE_BYTES_CHECKED_READER(u16)
LONE_BYTES_CHECKED_READER(s16)
LONE_BYTES_CHECKED_READER(u32)
LONE_BYTES_CHECKED_READER(s32)
LONE_BYTES_CHECKED_READER(u64)
LONE_BYTES_CHECKED_READER(s64)

LONE_BYTES_CHECKED_WRITER(u8)
LONE_BYTES_CHECKED_WRITER(s8)
LONE_BYTES_CHECKED_WRITER(u16)
LONE_BYTES_CHECKED_WRITER(s16)
LONE_BYTES_CHECKED_WRITER(u32)
LONE_BYTES_CHECKED_WRITER(s32)
LONE_BYTES_CHECKED_WRITER(u64)
LONE_BYTES_CHECKED_WRITER(s64)

#undef LONE_BYTES_CHECKED_READER
#undef LONE_BYTES_CHECKED_WRITER
