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

lone_u16 lone_u16_read_le(void *address)
{
	unsigned char *bytes = address;
	lone_u16 value = 0;

	value |= ((lone_u16) bytes[0]) << 0;
	value |= ((lone_u16) bytes[1]) << 8;

	return value;
}

lone_u16 lone_u16_read_be(void *address)
{
	unsigned char *bytes = address;
	lone_u16 value = 0;

	value |= ((lone_u16) bytes[0]) << 8;
	value |= ((lone_u16) bytes[1]) << 0;

	return value;
}

lone_u32 lone_u32_read_le(void *address)
{
	unsigned char *bytes = address;
	lone_u32 value = 0;

	value |= ((lone_u32) bytes[0]) <<  0;
	value |= ((lone_u32) bytes[1]) <<  8;
	value |= ((lone_u32) bytes[2]) << 16;
	value |= ((lone_u32) bytes[3]) << 24;

	return value;
}

lone_u32 lone_u32_read_be(void *address)
{
	unsigned char *bytes = address;
	lone_u32 value = 0;

	value |= ((lone_u32) bytes[0]) << 24;
	value |= ((lone_u32) bytes[1]) << 16;
	value |= ((lone_u32) bytes[2]) <<  8;
	value |= ((lone_u32) bytes[3]) <<  0;

	return value;
}

lone_u64 lone_u64_read_le(void *address)
{
	unsigned char *bytes = address;
	lone_u64 value = 0;

	value |= ((lone_u64) bytes[0]) <<  0;
	value |= ((lone_u64) bytes[1]) <<  8;
	value |= ((lone_u64) bytes[2]) << 16;
	value |= ((lone_u64) bytes[3]) << 24;
	value |= ((lone_u64) bytes[4]) << 32;
	value |= ((lone_u64) bytes[5]) << 40;
	value |= ((lone_u64) bytes[6]) << 48;
	value |= ((lone_u64) bytes[7]) << 56;

	return value;
}

lone_u64 lone_u64_read_be(void *address)
{
	unsigned char *bytes = address;
	lone_u64 value = 0;

	value |= ((lone_u64) bytes[0]) << 56;
	value |= ((lone_u64) bytes[1]) << 48;
	value |= ((lone_u64) bytes[2]) << 40;
	value |= ((lone_u64) bytes[3]) << 32;
	value |= ((lone_u64) bytes[4]) << 24;
	value |= ((lone_u64) bytes[5]) << 16;
	value |= ((lone_u64) bytes[6]) <<  8;
	value |= ((lone_u64) bytes[7]) <<  0;

	return value;
}

void lone_u16_write_le(void *address, lone_u16 value)
{
	unsigned char *bytes = address;

	bytes[0] = ((unsigned char) (value >>  0));
	bytes[1] = ((unsigned char) (value >>  8));
}

void lone_u16_write_be(void *address, lone_u16 value)
{
	unsigned char *bytes = address;

	bytes[0] = ((unsigned char) (value >>  8));
	bytes[1] = ((unsigned char) (value >>  0));
}

void lone_u32_write_le(void *address, lone_u32 value)
{
	unsigned char *bytes = address;

	bytes[0] = ((unsigned char) (value >>  0));
	bytes[1] = ((unsigned char) (value >>  8));
	bytes[2] = ((unsigned char) (value >> 16));
	bytes[3] = ((unsigned char) (value >> 24));
}

void lone_u32_write_be(void *address, lone_u32 value)
{
	unsigned char *bytes = address;

	bytes[0] = ((unsigned char) (value >> 24));
	bytes[1] = ((unsigned char) (value >> 16));
	bytes[2] = ((unsigned char) (value >>  8));
	bytes[3] = ((unsigned char) (value >>  0));
}

void lone_u64_write_le(void *address, lone_u64 value)
{
	unsigned char *bytes = address;

	bytes[0] = ((unsigned char) (value >>  0));
	bytes[1] = ((unsigned char) (value >>  8));
	bytes[2] = ((unsigned char) (value >> 16));
	bytes[3] = ((unsigned char) (value >> 24));
	bytes[4] = ((unsigned char) (value >> 32));
	bytes[5] = ((unsigned char) (value >> 40));
	bytes[6] = ((unsigned char) (value >> 48));
	bytes[7] = ((unsigned char) (value >> 56));
}

void lone_u64_write_be(void *address, lone_u64 value)
{
	unsigned char *bytes = address;

	bytes[0] = ((unsigned char) (value >> 56));
	bytes[1] = ((unsigned char) (value >> 48));
	bytes[2] = ((unsigned char) (value >> 40));
	bytes[3] = ((unsigned char) (value >> 32));
	bytes[4] = ((unsigned char) (value >> 24));
	bytes[5] = ((unsigned char) (value >> 16));
	bytes[6] = ((unsigned char) (value >>  8));
	bytes[7] = ((unsigned char) (value >>  0));
}

#define LONE_ENDIAN_SIGNED_READER(bits, endian)                                                    \
lone_s##bits lone_s##bits##_read_##endian(void *address)                                           \
{                                                                                                  \
	return (lone_s##bits) lone_u##bits##_read_##endian(address);                               \
}

#define LONE_ENDIAN_SIGNED_WRITER(bits, endian)                                                    \
void lone_s##bits##_write_##endian(void *address, lone_s##bits value)                              \
{                                                                                                  \
	lone_u##bits##_write_##endian(address, (lone_u##bits) value);                              \
}

#define LONE_ENDIAN_SIGNED_READERS_FOR_BITS(bits)                                                  \
LONE_ENDIAN_SIGNED_READER(bits, le)                                                                \
LONE_ENDIAN_SIGNED_READER(bits, be)                                                                \

#define LONE_ENDIAN_SIGNED_READERS()                                                               \
LONE_ENDIAN_SIGNED_READERS_FOR_BITS(16)                                                            \
LONE_ENDIAN_SIGNED_READERS_FOR_BITS(32)                                                            \
LONE_ENDIAN_SIGNED_READERS_FOR_BITS(64)                                                            \

LONE_ENDIAN_SIGNED_READERS()

LONE_ENDIAN_SIGNED_WRITER(16, le)
LONE_ENDIAN_SIGNED_WRITER(16, be)
LONE_ENDIAN_SIGNED_WRITER(32, le)
LONE_ENDIAN_SIGNED_WRITER(32, be)
LONE_ENDIAN_SIGNED_WRITER(64, le)
LONE_ENDIAN_SIGNED_WRITER(64, be)

#undef LONE_ENDIAN_SIGNED_READER
#undef LONE_ENDIAN_SIGNED_READERS_FOR_BITS
#undef LONE_ENDIAN_SIGNED_READERS
#undef LONE_ENDIAN_SIGNED_WRITER
