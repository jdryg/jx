#include <jx/utf8.h>

namespace jx
{
/*
* This may not be the best value, but it's one that isn't represented
*  in Unicode (0x10FFFF is the largest codepoint value). We return this
*  value from utf8codepoint() if there's bogus bits in the
*  stream. utf8codepoint() will turn this value into something
*  reasonable (like a question mark), for text that wants to try to recover,
*  whereas utf8valid() will use the value to determine if a string has bad
*  bits.
*/
#define UNICODE_BOGUS_CHAR_VALUE 0xFFFFFFFF

/*
* This is the codepoint we currently return when there was bogus bits in a
*  UTF-8 string. May not fly in Asian locales?
*/
#define UNICODE_BOGUS_CHAR_CODEPOINT '?'

uint32_t utf8ToCodepoint(const char** _str)
{
	const char* str = *_str;
	uint32_t retval = 0;
	uint32_t octet = (uint32_t)((uint8_t)*str);
	uint32_t octet2, octet3, octet4;

	if (octet == 0) {
		/* null terminator, end of string. */
		return 0;
	} else if (octet < 128)  {
		/* one octet char: 0 to 127 */
		(*_str)++;  /* skip to next possible start of codepoint. */
		return octet;
	} else if ((octet > 127) && (octet < 192)) {
		/* bad (starts with 10xxxxxx). */
		/*
		* Apparently each of these is supposed to be flagged as a bogus
		*  char, instead of just resyncing to the next valid codepoint.
		*/
		(*_str)++;  /* skip to next possible start of codepoint. */
		return UNICODE_BOGUS_CHAR_VALUE;
	} else if (octet < 224) {
		/* two octets */
		(*_str)++;  /* advance at least one byte in case of an error */
		octet -= (128 + 64);
		octet2 = (uint32_t)((uint8_t) *(++str));
		if ((octet2 & (128 + 64)) != 128) {
			/* Format isn't 10xxxxxx? */
			return UNICODE_BOGUS_CHAR_VALUE;
		}

		*_str += 1;  /* skip to next possible start of codepoint. */
		retval = ((octet << 6) | (octet2 - 128));
		if ((retval >= 0x80) && (retval <= 0x7FF)) {
			return retval;
		}
	} else if (octet < 240) {
		/* three octets */
		(*_str)++;  /* advance at least one byte in case of an error */
		octet -= (128 + 64 + 32);
		octet2 = (uint32_t)((uint8_t) *(++str));
		if ((octet2 & (128 + 64)) != 128) {
			/* Format isn't 10xxxxxx? */
			return UNICODE_BOGUS_CHAR_VALUE;
		}

		octet3 = (uint32_t)((uint8_t) *(++str));
		if ((octet3 & (128 + 64)) != 128) {
			/* Format isn't 10xxxxxx? */
			return UNICODE_BOGUS_CHAR_VALUE;
		}

		*_str += 2;  /* skip to next possible start of codepoint. */
		retval = (((octet << 12)) | ((octet2 - 128) << 6) | ((octet3 - 128)));

		/* There are seven "UTF-16 surrogates" that are illegal in UTF-8. */
		switch (retval)
		{
		case 0xD800:
		case 0xDB7F:
		case 0xDB80:
		case 0xDBFF:
		case 0xDC00:
		case 0xDF80:
		case 0xDFFF:
			return UNICODE_BOGUS_CHAR_VALUE;
		}

		/* 0xFFFE and 0xFFFF are illegal, too, so we check them at the edge. */
		if ((retval >= 0x800) && (retval <= 0xFFFD)) {
			return retval;
		}
	} else if (octet < 248) {
		/* four octets */
		(*_str)++;  /* advance at least one byte in case of an error */
		octet -= (128 + 64 + 32 + 16);
		octet2 = (uint32_t)((uint8_t) *(++str));
		if ((octet2 & (128 + 64)) != 128) {
			/* Format isn't 10xxxxxx? */
			return UNICODE_BOGUS_CHAR_VALUE;
		}

		octet3 = (uint32_t)((uint8_t) *(++str));
		if ((octet3 & (128 + 64)) != 128) {
			/* Format isn't 10xxxxxx? */
			return UNICODE_BOGUS_CHAR_VALUE;
		}

		octet4 = (uint32_t)((uint8_t) *(++str));
		if ((octet4 & (128 + 64)) != 128) {
			/* Format isn't 10xxxxxx? */
			return UNICODE_BOGUS_CHAR_VALUE;
		}

		*_str += 3;  /* skip to next possible start of codepoint. */
		retval = (((octet << 18)) | ((octet2 - 128) << 12) |
			((octet3 - 128) << 6) | ((octet4 - 128)));
		if ((retval >= 0x10000) && (retval <= 0x10FFFF)) {
			return retval;
		}
	} else if (octet < 252) {
		/*
		* Five and six octet sequences became illegal in rfc3629.
		*  We throw the codepoint away, but parse them to make sure we move
		*  ahead the right number of bytes and don't overflow the buffer.
		*/
		/* five octets */
		(*_str)++;  /* advance at least one byte in case of an error */
		octet = (uint32_t)((uint8_t) *(++str));
		if ((octet & (128 + 64)) != 128) {
			/* Format isn't 10xxxxxx? */
			return UNICODE_BOGUS_CHAR_VALUE;
		}

		octet = (uint32_t)((uint8_t) *(++str));
		if ((octet & (128 + 64)) != 128) {
			/* Format isn't 10xxxxxx? */
			return UNICODE_BOGUS_CHAR_VALUE;
		}

		octet = (uint32_t)((uint8_t) *(++str));
		if ((octet & (128 + 64)) != 128) {
			/* Format isn't 10xxxxxx? */
			return UNICODE_BOGUS_CHAR_VALUE;
		}

		octet = (uint32_t)((uint8_t) *(++str));
		if ((octet & (128 + 64)) != 128) {
			/* Format isn't 10xxxxxx? */
			return UNICODE_BOGUS_CHAR_VALUE;
		}

		*_str += 4;  /* skip to next possible start of codepoint. */
		return UNICODE_BOGUS_CHAR_VALUE;
	} else {
		/* six octets */
		(*_str)++;  /* advance at least one byte in case of an error */
		octet = (uint32_t)((uint8_t) *(++str));
		if ((octet & (128 + 64)) != 128) {
			/* Format isn't 10xxxxxx? */
			return UNICODE_BOGUS_CHAR_VALUE;
		}

		octet = (uint32_t)((uint8_t) *(++str));
		if ((octet & (128 + 64)) != 128) {
			/* Format isn't 10xxxxxx? */
			return UNICODE_BOGUS_CHAR_VALUE;
		}

		octet = (uint32_t)((uint8_t) *(++str));
		if ((octet & (128 + 64)) != 128) {
			/* Format isn't 10xxxxxx? */
			return UNICODE_BOGUS_CHAR_VALUE;
		}

		octet = (uint32_t)((uint8_t) *(++str));
		if ((octet & (128 + 64)) != 128) {
			/* Format isn't 10xxxxxx? */
			return UNICODE_BOGUS_CHAR_VALUE;
		}

		octet = (uint32_t)((uint8_t) *(++str));
		if ((octet & (128 + 64)) != 128) {
			/* Format isn't 10xxxxxx? */
			return UNICODE_BOGUS_CHAR_VALUE;
		}

		*_str += 6;  /* skip to next possible start of codepoint. */
		return UNICODE_BOGUS_CHAR_VALUE;
	}

	return UNICODE_BOGUS_CHAR_VALUE;
}

uint32_t utf16ToCodepoint(const uint16_t**_str)
{
	const uint16_t* src = *_str;
	uint32_t cp = (uint32_t)*(src++);

	if (cp == 0) {
		/* null terminator, end of string. */
		return 0;
	} else if ((cp >= 0xDC00) && (cp <= 0xDFFF)) {
		/* Orphaned second half of surrogate pair? */
		cp = UNICODE_BOGUS_CHAR_CODEPOINT;
	} else if ((cp >= 0xD800) && (cp <= 0xDBFF))  {
		/* start surrogate pair! */
		const uint32_t pair = (uint32_t)*src;
		if (pair == 0) {
			cp = UNICODE_BOGUS_CHAR_CODEPOINT;
		} else if ((pair < 0xDC00) || (pair > 0xDFFF)) {
			cp = UNICODE_BOGUS_CHAR_CODEPOINT;
		} else {
			src++;  /* eat the other surrogate. */
			cp = (((cp - 0xD800) << 10) | (pair - 0xDC00));
		}
	}

	*_str = src;

	return cp;
}

void utf8FromCodepoint(uint32_t cp, char** _dst, uint32_t* _len)
{
	char* dst = *_dst;
	uint32_t len = *_len;

	if (len == 0) {
		return;
	}

	if (cp > 0x10FFFF) {
		cp = UNICODE_BOGUS_CHAR_CODEPOINT;
	} else if ((cp == 0xFFFE) || (cp == 0xFFFF)) {
		/* illegal values. */
		cp = UNICODE_BOGUS_CHAR_CODEPOINT;
	} else {
		/* There are seven "UTF-16 surrogates" that are illegal in UTF-8. */
		switch (cp) {
		case 0xD800:
		case 0xDB7F:
		case 0xDB80:
		case 0xDBFF:
		case 0xDC00:
		case 0xDF80:
		case 0xDFFF:
			cp = UNICODE_BOGUS_CHAR_CODEPOINT;
		}
	}

	/* Do the encoding... */
	if (cp < 0x80) {
		*(dst++) = (char)cp;
		len--;
	} else if (cp < 0x800) {
		if (len < 2) {
			len = 0;
		} else {
			*(dst++) = (char)((cp >> 6) | 128 | 64);
			*(dst++) = (char)(cp & 0x3F) | 128;
			len -= 2;
		}
	} else if (cp < 0x10000) {
		if (len < 3) {
			len = 0;
		} else {
			*(dst++) = (char)((cp >> 12) | 128 | 64 | 32);
			*(dst++) = (char)((cp >> 6) & 0x3F) | 128;
			*(dst++) = (char)(cp & 0x3F) | 128;
			len -= 3;
		}
	} else {
		if (len < 4) {
			len = 0;
		} else {
			*(dst++) = (char)((cp >> 18) | 128 | 64 | 32 | 16);
			*(dst++) = (char)((cp >> 12) & 0x3F) | 128;
			*(dst++) = (char)((cp >> 6) & 0x3F) | 128;
			*(dst++) = (char)(cp & 0x3F) | 128;
			len -= 4;
		}
	}

	*_dst = dst;
	*_len = len;
}

void utf8FromUtf16(const uint16_t* src, char* dst, uint32_t len)
{
	if (len == 0) {
		return;
	}

	len--;
	while (len) {
		const uint32_t cp = utf16ToCodepoint(&src);
		if (!cp) {
			break;
		}

		utf8FromCodepoint(cp, &dst, &len);
	}

	*dst = '\0';
}

void utf8ToUtf16(const char* src, uint16_t* dst, uint32_t len)
{
	len -= sizeof(uint16_t);   /* save room for null char. */
	while (len >= sizeof(uint16_t)) {
		uint32_t cp = utf8ToCodepoint(&src);
		if (cp == 0) {
			break;
		} else if (cp == UNICODE_BOGUS_CHAR_VALUE) {
			cp = UNICODE_BOGUS_CHAR_CODEPOINT;
		}

		if (cp > 0xFFFF) {
			/* encode as surrogate pair */
			if (len < (sizeof(uint16_t) * 2)) {
				break;  /* not enough room for the pair, stop now. */
			}

			cp -= 0x10000;  /* Make this a 20-bit value */

			*(dst++) = 0xD800 + ((cp >> 10) & 0x3FF);
			len -= sizeof(uint16_t);

			cp = 0xDC00 + (cp & 0x3FF);
		}

		*(dst++) = (uint16_t)cp;
		len -= sizeof(uint16_t);
	}

	*dst = 0;
}

void utf8FromCodepoint(uint32_t cp, char* str)
{
	uint32_t len = 8;
	utf8FromCodepoint(cp, &str, &len);
	*str = '\0';
}
}
