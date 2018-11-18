#ifndef JX_COLOR_H
#define JX_COLOR_H

#include <stdint.h>

namespace jx
{
uint32_t hueToABGR(float hue);
float hueFromABGR(uint32_t abgr);
void HSBFromABGR(uint32_t abgr, float* hue, float* sat, float* brightness);
uint32_t HSBToABGR(float hue, float sat, float brightness);
}

#include "inline/color.inl"

#endif
