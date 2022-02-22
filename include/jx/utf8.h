#ifndef JX_UTF8_H
#define JX_UTF8_H

#include <stdint.h>

namespace jx
{
void utf8FromUtf16(const uint16_t* src, char* dst, uint32_t len);
void utf8ToUtf16(const char* src, uint16_t* dst, uint32_t len);

uint32_t utf8ToCodepoint(const char** _str);
uint32_t utf16ToCodepoint(const uint16_t**_str);
void utf8FromCodepoint(uint32_t cp, char** _dst, uint32_t* _len);

uint32_t utf8Ellipsize(const char* src, uint32_t maxChars, char* dst, uint32_t dstSize);

uint32_t utf8FindPrevChar(const char* str, uint32_t len, uint32_t start);
uint32_t utf8FindNextChar(const char* str, uint32_t len, uint32_t start);
uint32_t utf8FindPrevWhitespace(const char* str, uint32_t len, uint32_t start);
uint32_t utf8FindNextWhitespace(const char* str, uint32_t len, uint32_t start);
uint32_t utf8StrLen(const char* start, const char* end);
uint32_t utf8GetCodepointPos(const char* start, const char* end, uint32_t numCodepoints);
uint32_t utf8Insert(char* buffer, uint32_t bufferLen, uint32_t bufferMaxLen, int cursor, const char* utf8, uint32_t utf8Len);
uint32_t utf8Erase(char* buffer, uint32_t len, int start, int end);

inline bool utf8IsStartByte(uint8_t ch)
{
	return (ch & 0x80) == 0x00  // ASCII
		|| (ch & 0xC0) == 0xC0; // UTF-8 first byte
}
}

#endif
