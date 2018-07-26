#ifndef JX_MATH_H
#define JX_MATH_H

#include <stdint.h>

namespace jx
{
uint32_t nextPowerOf2(uint32_t v);
bool isPowerOf2(uint32_t v);
uint32_t log2ui(uint32_t v);
}

#include "inline/math.inl"

#endif
