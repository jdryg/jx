#ifndef JX_VEC2_H
#error "Must be included from jx/vec2.h"
#endif

namespace jx
{
inline Vec2f vec2f(float x, float y)
{
	return { x, y };
}

inline void vec2fSet(Vec2f& v, float x, float y)
{
	v.x = x;
	v.y = y;
}

inline float vec2fDistanceSqr(const Vec2f& a, const Vec2f& b)
{
	const float dx = b.x - a.x;
	const float dy = b.y - a.y;
	return dx * dx + dy * dy;
}

inline Vec2d vec2d(double x, double y)
{
	return { x, y };
}

inline void vec2dSet(Vec2d& v, double x, double y)
{
	v.x = x;
	v.y = y;
}

inline double vec2dDistanceSqr(const Vec2d& a, const Vec2d& b)
{
	const double dx = b.x - a.x;
	const double dy = b.y - a.y;
	return dx * dx + dy * dy;
}
}
