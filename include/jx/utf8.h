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

void utf8FromCodepoint(uint32_t cp, char* str);

uint32_t utf8Ellipsize(const char* src, uint32_t maxChars, char* dst, uint32_t dstSize);
}

#endif
