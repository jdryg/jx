#ifndef JX_VEC2_H
#define JX_VEC2_H

#include <stdint.h>

namespace bx
{
struct AllocatorI;
}

namespace jx
{
struct Vec2
{
	float x, y;
};

struct Vec2Array
{
	bx::AllocatorI* m_Allocator;
	Vec2* m_Pts;
	uint32_t m_Size;
	uint32_t m_Capacity;
};

Vec2 vec2(float x, float y);
void vec2Set(Vec2& v, float x, float y);
float vec2DistanceSqr(const Vec2& a, const Vec2& b);

bool vec2ArrInit(Vec2Array* arr, uint32_t n, bx::AllocatorI* allocator = nullptr);
void vec2ArrDestroy(Vec2Array* arr);
bool vec2ArrResize(Vec2Array* arr, uint32_t n);
}

#include "inline/vec2.inl"

#endif
