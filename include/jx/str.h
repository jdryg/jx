#ifndef JX_STRING_H
#define JX_STRING_H

#include <stdint.h>

namespace jx
{
char* strDup(const char* str, uint32_t len = UINT32_MAX);
void strFree(char* str);
char* strReplace(const char* str, uint32_t len, const char* from, const char* to);
bool strEllipsize(char* str, uint32_t maxLen);

int strCmpIN(const char* str1, const char* str2, uint32_t len);
int strCmpI(const char* str1, const char* str2);
}

#endif
