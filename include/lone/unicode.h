/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_UNICODE_HEADER
#define LONE_UNICODE_HEADER

#include <lone/types.h>

/* ╭──────────────────────────────┨ UNICODE ┠───────────────────────────────╮
   │                                                                        │
   │    UTF-8 decoder, encoder and validator.                               │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

#define LONE_UNICODE_CODE_POINT_MAX          0x10FFFF
#define LONE_UNICODE_SURROGATE_MIN           0xD800
#define LONE_UNICODE_SURROGATE_MAX           0xDFFF

struct lone_unicode_utf8_decode_result {
	lone_u32 code_point;
	lone_u8  bytes_read;
};

struct lone_unicode_utf8_encode_result {
	unsigned char bytes[4];
	lone_u8       bytes_written;
};

struct lone_unicode_utf8_validation_result {
	size_t code_point_count;
	bool valid;
};

bool lone_unicode_is_valid_code_point(lone_u32 code_point);
struct lone_unicode_utf8_decode_result lone_unicode_utf8_decode(struct lone_bytes bytes);
struct lone_unicode_utf8_encode_result lone_unicode_utf8_encode(lone_u32 code_point);

#endif /* LONE_UNICODE_HEADER */
