#ifndef JX_MEMORY_STREAM_H
#error "Must be included from jx/memory_stream.h"
#endif

#include <bx/string.h>

namespace jx
{
inline uint8_t memStreamRead_uint8(MemoryStream* ms)
{
	uint8_t val;
	memStreamRead(ms, &val, sizeof(uint8_t));
	return val;
}

inline uint32_t memStreamRead_uint32(MemoryStream* ms)
{
	uint32_t val;
	memStreamRead(ms, &val, sizeof(uint32_t));
	return val;
}

inline bool memStreamRead_string(MemoryStream* ms, char* str, uint32_t maxLen)
{
	uint32_t len;
	if (!memStreamRead(ms, &len, sizeof(uint32_t))) {
		return false;
	}

	const uint32_t readLen = len < (maxLen - 1) ? len : (maxLen - 1);
	if (!memStreamRead(ms, str, readLen)) {
		return false;
	}
	str[readLen] = '\0';

	if (readLen < len) {
		memStreamGetReadPtr(ms, len - readLen);
	}

	return true;
}

inline bool memStreamWrite_uint8(MemoryStream* ms, uint8_t val)
{
	return memStreamWrite(ms, &val, sizeof(uint8_t));
}

inline bool memStreamWrite_uint32(MemoryStream* ms, uint32_t val)
{
	return memStreamWrite(ms, &val, sizeof(uint32_t));
}

inline bool memStreamWrite_string(MemoryStream* ms, const char* str)
{
	const uint32_t len = bx::strLen(str);
	if (!memStreamWrite(ms, &len, sizeof(uint32_t))) {
		return false;
	}

	if (!memStreamWrite(ms, str, len)) {
		return false;
	}

	return true;
}
}
