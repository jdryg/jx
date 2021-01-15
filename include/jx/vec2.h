#ifndef JX_VEC2_H
#define JX_VEC2_H

#include <stdint.h>

namespace bx
{
struct AllocatorI;
}

namespace jx
{
struct Vec2f
{
	float x, y;
};

struct Vec2d
{
	double x, y;
};

struct Vec2fArray
{
	bx::AllocatorI* m_Allocator;
	Vec2f* m_Pts;
	uint32_t m_Size;
	uint32_t m_Capacity;
};

struct Vec2dArray
{
	bx::AllocatorI* m_Allocator;
	Vec2d* m_Pts;
	uint32_t m_Size;
	uint32_t m_Capacity;
};

Vec2f vec2f(float x, float y);
void vec2fSet(Vec2f& v, float x, float y);
float vec2fDistanceSqr(const Vec2f& a, const Vec2f& b);

Vec2d vec2d(double x, double y);
void vec2dSet(Vec2d& v, double x, double y);
double vec2dDistanceSqr(const Vec2d& a, const Vec2d& b);

bool vec2fArrInit(Vec2fArray* arr, uint32_t n, bx::AllocatorI* allocator = nullptr);
void vec2fArrDestroy(Vec2fArray* arr);
bool vec2fArrResize(Vec2fArray* arr, uint32_t n);
bool vec2fArrFromScalar(Vec2fArray* arr, const float* x, const float* y, uint32_t n);
uint32_t vec2fArrClosestPoint(const Vec2fArray* arr, const Vec2f& v, float* dist);
void vec2fArrRemovePoint(Vec2fArray* arr, uint32_t id);
bool vec2fArrCalcBoundingRect(const Vec2fArray* arr, Vec2f& minPt, Vec2f& maxPt);

bool vec2dArrInit(Vec2dArray* arr, uint32_t n, bx::AllocatorI* allocator = nullptr);
void vec2dArrDestroy(Vec2dArray* arr);
bool vec2dArrResize(Vec2dArray* arr, uint32_t n);
bool vec2dArrFromScalar(Vec2dArray* arr, const double* x, const double* y, uint32_t n);

bool vec2Arrd2f(Vec2fArray* dst, const Vec2dArray* src);
}

#include "inline/vec2.inl"

#endif
