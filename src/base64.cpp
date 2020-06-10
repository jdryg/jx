#include <jx/base64.h>
#include <bx/allocator.h>

namespace jx
{
// https://web.mit.edu/freebsd/head/contrib/wpa/src/utils/base64.c
/*
 * Base64 encoding/decoding (RFC1341)
 * Copyright (c) 2005-2011, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */
static const uint8_t kBase64Table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
#define _ 0x80
static const uint8_t kBase64CharOffset[256] = {
	 _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
	 _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
	 _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _, 62,  _,  _,  _, 63,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  _,  _,  _,  0,  _,  _,
	 _,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  _,  _,  _,  _,  _,
	 _, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,  _,  _,  _,  _,  _,
	 _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
	 _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
	 _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
	 _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
	 _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
	 _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
	 _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
	 _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,  _,
};
#undef _

/**
 * base64_encode - Base64 encode
 * @src: Data to be encoded
 * @len: Length of the data to be encoded
 * @out_len: Pointer to output length variable, or %NULL if not used
 * Returns: Allocated buffer of out_len bytes of encoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer. Returned buffer is
 * nul terminated to make it easier to use as a C string. The nul terminator is
 * not included in out_len.
 */
uint8_t* base64Encode(const uint8_t* src, size_t len, size_t* out_len, bx::AllocatorI* allocator)
{
	size_t olen = len * 4 / 3 + 4; /* 3-byte blocks to 4-byte */
	olen += olen / 72; /* line feeds */
	olen++; /* nul termination */
	if (olen < len) {
		return nullptr; /* integer overflow */
	}

	uint8_t* out = (uint8_t*)BX_ALLOC(allocator, olen);
	if (out == nullptr) {
		return nullptr;
	}

	const uint8_t* end = src + len;
	const uint8_t* in = src;
	uint8_t* pos = out;
	int line_len = 0;
	while (end - in >= 3) {
		*pos++ = kBase64Table[in[0] >> 2];
		*pos++ = kBase64Table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
		*pos++ = kBase64Table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
		*pos++ = kBase64Table[in[2] & 0x3f];

		in += 3;

		line_len += 4;
		if (line_len >= 72) {
			*pos++ = '\n';
			line_len = 0;
		}
	}

	if (end - in) {
		*pos++ = kBase64Table[in[0] >> 2];
		if (end - in == 1) {
			*pos++ = kBase64Table[(in[0] & 0x03) << 4];
			*pos++ = '=';
		} else {
			*pos++ = kBase64Table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
			*pos++ = kBase64Table[(in[1] & 0x0f) << 2];
		}
		*pos++ = '=';
		line_len += 4;
	}

	if (line_len) {
		*pos++ = '\n';
	}

	*pos = '\0';
	if (out_len) {
		*out_len = pos - out;
	}

	return out;
}

/**
 * base64_decode - Base64 decode
 * @src: Data to be decoded
 * @len: Length of the data to be decoded
 * @out_len: Pointer to output length variable
 * Returns: Allocated buffer of out_len bytes of decoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer.
 */
uint8_t* base64Decode(const uint8_t* src, size_t len, size_t* out_len, bx::AllocatorI* allocator)
{
	size_t count = 0;
	for (size_t i = 0; i < len; i++) {
		if (kBase64CharOffset[src[i]] != 0x80) {
			count++;
		}
	}

	if (count == 0 || count % 4) {
		return nullptr;
	}

	size_t olen = count / 4 * 3;
	uint8_t* out = (uint8_t*)BX_ALLOC(allocator, olen);
	if (out == nullptr) {
		return nullptr;
	}
	uint8_t* pos = out;

	uint8_t block[4];
	int pad = 0;
	count = 0;
	for (size_t i = 0; i < len; i++) {
		uint8_t tmp = kBase64CharOffset[src[i]];
		if (tmp == 0x80) {
			continue;
		}

		if (src[i] == '=') {
			pad++;
		}

		block[count] = tmp;
		count++;
		if (count == 4) {
			*pos++ = (block[0] << 2) | (block[1] >> 4);
			*pos++ = (block[1] << 4) | (block[2] >> 2);
			*pos++ = (block[2] << 6) | block[3];

			count = 0;

			if (pad) {
				if (pad == 1) {
					pos--;
				} else if (pad == 2) {
					pos -= 2;
				} else {
					/* Invalid padding */
					BX_FREE(allocator, out);
					return nullptr;
				}

				break;
			}
		}
	}

	*out_len = pos - out;

	return out;
}
}
