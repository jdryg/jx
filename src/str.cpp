#include <jx/str.h>
#include <jx/sys.h>
#include <bx/allocator.h>
#include <bx/string.h>
#include <bx/uint32_t.h>

namespace jx
{
char* strDup(bx::AllocatorI* allocator, const char* str, uint32_t len)
{
	len = len == UINT32_MAX ? (uint32_t)bx::strLen(str) : len;

	char* cpy = (char*)BX_ALLOC(allocator, sizeof(char) * (len + 1));
	bx::memCopy(cpy, str, sizeof(char) * len);
	cpy[len] = '\0';

	return cpy;
}

void strFree(bx::AllocatorI* allocator, char* str)
{
	BX_FREE(allocator, str);
}

char* strDup(const char* str, uint32_t len)
{
	return strDup(getGlobalAllocator(), str, len);
}

void strFree(char* str)
{
	strFree(getGlobalAllocator(), str);
}

char* strReplace(const char* str, uint32_t len, const char* from, const char* to)
{
	len = len == UINT32_MAX ? (uint32_t)bx::strLen(str) : len;
	const uint32_t fromLen = (uint32_t)bx::strLen(from);
	const uint32_t toLen = (uint32_t)bx::strLen(to);

	if (len == 0 || fromLen == 0) {
		return strDup(str, len);
	}

	// Find the first match
	const char* match = bx::strFind(str, from);

	// If there's no match or the match is beyond the end of the string, just duplicate the input string.
	if (!match || (match - str) >= (int)len) {
		return strDup(str, len);
	}

	// At least one match. Predict the size of the new string based on the sizes of from and to...
	size_t dstCapacity = (fromLen >= toLen) ? len : len + (toLen - fromLen) * 4;

	bx::AllocatorI* allocator = getGlobalAllocator();
	char* finalString = (char*)BX_ALLOC(allocator, sizeof(char) * (dstCapacity + 1));

	const char* src = str;
	char* dst = finalString;

	while (match) {
		const size_t curPos = (size_t)(dst - finalString);
		size_t matchRelPos = (size_t)(match - src);

		// Check if the match is still inside the current string bounds.
		bool appendTo = true;
		if (curPos + matchRelPos >= (size_t)len) {
			// Copy only the part of the string we are interested in.
			matchRelPos = len - curPos;
			appendTo = false;
		}

		// Check if there's enough space from the substring...
		if (curPos + matchRelPos + toLen >= dstCapacity) {
			dstCapacity = bx::uint32_max((uint32_t)((dstCapacity * 3) >> 1), (uint32_t)(curPos + matchRelPos + toLen));
			finalString = (char*)BX_REALLOC(allocator, finalString, sizeof(char) * (dstCapacity + 1));
			dst = finalString + curPos;
		}

		// Copy the substrings...
		bx::memCopy(dst, src, sizeof(char) * matchRelPos);
		dst += matchRelPos;
		if (appendTo) {
			bx::memCopy(dst, to, sizeof(char) * toLen);
			dst += toLen;
		}

		src += matchRelPos + fromLen;

		// Find the new match...
		match = bx::strFind(src, from);
		if (match - str >= (int)len) {
			break;
		}
	}

	size_t curPos = (size_t)(src - str);
	if (curPos < (size_t)len) {
		bx::memCopy(dst, src, (size_t)len - curPos);
		dst += len - curPos;
	}

	*dst = '\0';

	return finalString;
}

bool strEllipsize(char* str, uint32_t maxLen)
{
	uint32_t len = (uint32_t)bx::strLen(str);
	if (len > maxLen) {
		str[maxLen - 3] = '.';
		str[maxLen - 2] = '.';
		str[maxLen - 1] = '.';
		str[maxLen] = '\0';

		return true;
	}

	return false;
}

int strCmpIN(const char* str1, const char* str2, uint32_t len)
{
	return bx::strCmpI(bx::StringView(str1, len), bx::StringView(str2, len), len);
}

int strCmpI(const char* str1, const char* str2)
{
	return bx::strCmpI(bx::StringView(str1), bx::StringView(str2));
}
}
