#ifndef JX_MATH_H
#define JX_MATH_H

#include <stdint.h>
#include <bx/math.h>

namespace jx
{
uint32_t nextPowerOf2(uint32_t v);
bool isPowerOf2(uint32_t v);
uint32_t log2ui(uint32_t v);

float snapTo(float x, float stepSize);

bool closestPointOnLineSegment(float x, float y, float sx, float sy, float ex, float ey, float thickness, float* t);
}

#include "inline/math.inl"

#endif
