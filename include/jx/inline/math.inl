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

inline uint32_t log2ui(uint32_t v)
{
	uint32_t r = 0; // r will be lg(v)
	while (v >>= 1) {
		r++;
	}

	return r;
}

inline float snapTo(float x, float stepSize)
{
	int ix = (int)bx::round(x / stepSize);
	return ix * stepSize;
}

// http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetKernighan
inline uint32_t bitcount(uint32_t v)
{
	uint32_t c;
	for (c = 0; v; c++) {
		v &= v - 1;
	}
	return c;
}
}
