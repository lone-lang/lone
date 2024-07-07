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

lone_u8 lone_u8_read(void *address)
{
	lone_u8 *always_aligned = address;
	return *always_aligned;
}

lone_s8 lone_s8_read(void *address)
{
	lone_s8 *always_aligned = address;
	return *always_aligned;
}

void lone_u8_write(void *address, lone_u8 value)
{
	lone_u8 *always_aligned = address;
	*always_aligned = value;
}

void lone_s8_write(void *address, lone_s8 value)
{
	lone_s8 *always_aligned = address;
	*always_aligned = value;
}

#define LONE_READER(type) \
lone_##type lone_##type##_read(void *address) \
{ \
	lone_##type value; \
	lone_memory_move(address, &value, sizeof(lone_##type)); /* byte-wise copy, native endianness */ \
	return value; \
}

#define LONE_WRITER(type) \
void lone_##type##_write(void *address, lone_##type value) \
{ \
	lone_memory_move(&value, address, sizeof(lone_##type)); /* byte-wise copy, native endianness */ \
}

LONE_READER(u16)
LONE_READER(s16)
LONE_READER(u32)
LONE_READER(s32)
LONE_READER(u64)
LONE_READER(s64)

LONE_WRITER(u16)
LONE_WRITER(s16)
LONE_WRITER(u32)
LONE_WRITER(s32)
LONE_WRITER(u64)
LONE_WRITER(s64)

#undef LONE_READER
#undef LONE_WRITER

#define LONE_BYTES_READER(type) \
struct lone_##type lone_bytes_read_##type(struct lone_bytes bytes, lone_size offset)               \
{                                                                                                  \
	struct lone_##type result = { .present = false, .value = 0 };                              \
	                                                                                           \
	if (lone_bytes_contains_block(bytes, offset, sizeof(lone_##type))) {                       \
		result.value = lone_##type##_read(bytes.pointer + offset);                         \
		result.present = true;                                                             \
	}                                                                                          \
	                                                                                           \
	return result;                                                                             \
}

#define LONE_BYTES_WRITER(type) \
bool lone_bytes_write_##type(struct lone_bytes bytes, lone_size offset, lone_##type value)         \
{                                                                                                  \
	bool written = false;                                                                      \
	                                                                                           \
	if (lone_bytes_contains_block(bytes, offset, sizeof(lone_##type))) {                       \
		lone_##type##_write(bytes.pointer + offset, value);                                \
		written = true;                                                                    \
	}                                                                                          \
	                                                                                           \
	return written;                                                                            \
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
