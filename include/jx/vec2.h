#ifndef JX_VEC2_H
#define JX_VEC2_H

namespace jx
{
struct Vec2
{
	float x, y;
};

Vec2 vec2(float x, float y);
void vec2Set(Vec2& v, float x, float y);

float vec2DistanceSqr(const Vec2& a, const Vec2& b);
}

#include "inline/vec2.inl"

#endif
