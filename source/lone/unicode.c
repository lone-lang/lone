/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/unicode.h>

bool lone_unicode_is_valid_code_point(lone_u32 code_point)
{
	if (code_point > LONE_UNICODE_CODE_POINT_MAX) { return false; }
	if (   code_point >= LONE_UNICODE_SURROGATE_MIN
	    && code_point <= LONE_UNICODE_SURROGATE_MAX) { return false; }

	return true;
}

static bool is_continuation_byte(unsigned char byte)
{
	return (byte & 0xC0) == 0x80;
}

struct lone_unicode_utf8_decode_result lone_unicode_utf8_decode(struct lone_bytes bytes)
{
	struct lone_unicode_utf8_decode_result result = { 0, 0 };
	unsigned char leading;
	lone_u32 code_point;
	lone_u8 expected;
	lone_u8 i;

	if (bytes.count == 0) { return result; }

	leading = bytes.pointer[0];

	if (leading < 0x80) {
		result.code_point = leading;
		result.bytes_read = 1;
		return result;
	} else if ((leading & 0xE0) == 0xC0) {
		code_point = leading & 0x1F;
		expected = 2;
	} else if ((leading & 0xF0) == 0xE0) {
		code_point = leading & 0x0F;
		expected = 3;
	} else if ((leading & 0xF8) == 0xF0) {
		code_point = leading & 0x07;
		expected = 4;
	} else {
		return result;
	}

	if (bytes.count < expected) { return result; }

	for (i = 1; i < expected; ++i) {
		if (!is_continuation_byte(bytes.pointer[i])) {
			return result;
		}

		code_point = (code_point << 6) | (bytes.pointer[i] & 0x3F);
	}

	/* reject overlong encodings */
	switch (expected) {
	case 2:
		if (code_point < 0x80) { return result; }
		break;
	case 3:
		if (code_point < 0x800) { return result; }
		break;
	case 4:
		if (code_point < 0x10000) { return result; }
		break;
	}

	if (!lone_unicode_is_valid_code_point(code_point)) { return result; }

	result.code_point = code_point;
	result.bytes_read = expected;
	return result;
}
