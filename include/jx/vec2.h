#ifndef JX_VEC2_H
#define JX_VEC2_H

namespace jx
{
struct Vec2
{
	float x, y;
};

inline float vec2DistanceSqr(const Vec2& a, const Vec2& b)
{
	const float dx = b.x - a.x;
	const float dy = b.y - a.y;
	return dx * dx + dy * dy;
}
}

#endif
