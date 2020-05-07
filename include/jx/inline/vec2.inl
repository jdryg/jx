#ifndef JX_VEC2_H
#error "Must be included from jx/vec2.h"
#endif

namespace jx
{
inline Vec2 vec2(float x, float y)
{
	return { x, y };
}

inline void vec2Set(Vec2& v, float x, float y)
{
	v.x = x;
	v.y = y;
}

inline float vec2DistanceSqr(const Vec2& a, const Vec2& b)
{
	const float dx = b.x - a.x;
	const float dy = b.y - a.y;
	return dx * dx + dy * dy;
}
}
