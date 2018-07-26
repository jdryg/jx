#ifndef JX_MATH_H
#error "Must be included from jx/math.h"
#endif

namespace jx
{
inline uint32_t nextPowerOf2(uint32_t v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;

	return v;
}

inline bool isPowerOf2(uint32_t v)
{
	return ((v != 0) && !(v & (v - 1)));
}

inline uint32_t log2ui(uint32_t v)
{
	uint32_t r = 0; // r will be lg(v)
	while (v >>= 1) {
		r++;
	}

	return r;
}
}
