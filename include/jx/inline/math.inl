#ifndef JX_MATH_H
#error "Must be included from jx/math.h"
#endif

#include <math.h>

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

inline uint32_t solveQuadratic(const float* coefs, float* res)
{
	const float a = coefs[0];
	const float b = coefs[1];
	const float c = coefs[2];

	const float det = b * b - 4.0f * a * c;
	if (det < 0.0f) {
		return 0;
	} else if (det == 0.0f) {
		res[0] = -b / (2.0f * a);
		res[1] = res[0];
		return 1;
	}

	const float sqrt_det = bx::sqrt(det);
	const float denom = 1.0f / (2.0f * a);
	res[0] = (-b + sqrt_det) * denom;
	res[1] = (-b - sqrt_det) * denom;
	return 2;
}

inline double toDeg(double rad)
{
	return rad * 180.0 / kPid;
}

inline double toRad(double deg)
{
	return deg * kPid / 180.0;
}

inline double mod(double _a, double _b)
{
	return _a - _b * floor(_a / _b);
}

inline double wrap(double _a, double _wrap)
{
	const double tmp0 = mod(_a, _wrap);
	const double result = tmp0 < 0.0 
		? _wrap + tmp0 
		: tmp0
		;
	return result;
}
}
