#include <jx/vec2.h>
#include <jx/sys.h>
#include <bx/bx.h>
#include <bx/allocator.h>

namespace jx
{
bool vec2fArrInit(Vec2fArray* arr, uint32_t n, bx::AllocatorI* allocator)
{
	bx::memSet(arr, 0, sizeof(Vec2fArray));
	arr->m_Allocator = allocator != nullptr ? allocator : getGlobalAllocator();

	return n != 0 ? vec2fArrResize(arr, n) : true;
}

void vec2fArrDestroy(Vec2fArray* arr)
{
	bx::AllocatorI* allocator = arr->m_Allocator;
	BX_ALIGNED_FREE(allocator, arr->m_Pts, 16);
	arr->m_Pts = 0;
	arr->m_Size = 0;
	arr->m_Capacity = 0;
}

bool vec2fArrResize(Vec2fArray* arr, uint32_t n)
{
	if (n > arr->m_Capacity) {
		Vec2f* newPts = (Vec2f*)BX_ALIGNED_ALLOC(arr->m_Allocator, sizeof(Vec2f) * n, 16);
		if (newPts == nullptr) {
			return false;
		}

		bx::memCopy(newPts, arr->m_Pts, sizeof(Vec2f) * arr->m_Size);

		BX_ALIGNED_FREE(arr->m_Allocator, arr->m_Pts, 16);
		arr->m_Pts = newPts;
		arr->m_Capacity = n;
	}

	arr->m_Size = n;
	return true;
}

bool vec2fArrFromScalar(Vec2fArray* arr, const float* x, const float* y, uint32_t n)
{
	if (!vec2fArrResize(arr, n)) {
		return false;
	}
	
	Vec2f* pts = arr->m_Pts;
	for (uint32_t i = 0; i < n; ++i) {
		pts->x = *x++;
		pts->y = *y++;
		pts++;
	}

	return true;
}

bool vec2dArrInit(Vec2dArray* arr, uint32_t n, bx::AllocatorI* allocator)
{
	bx::memSet(arr, 0, sizeof(Vec2dArray));
	arr->m_Allocator = allocator != nullptr ? allocator : getGlobalAllocator();

	return n != 0 ? vec2dArrResize(arr, n) : true;
}

void vec2dArrDestroy(Vec2dArray* arr)
{
	bx::AllocatorI* allocator = arr->m_Allocator;
	BX_ALIGNED_FREE(allocator, arr->m_Pts, 16);
	arr->m_Pts = 0;
	arr->m_Size = 0;
	arr->m_Capacity = 0;
}

bool vec2dArrResize(Vec2dArray* arr, uint32_t n)
{
	if (n > arr->m_Capacity) {
		Vec2d* newPts = (Vec2d*)BX_ALIGNED_ALLOC(arr->m_Allocator, sizeof(Vec2d) * n, 16);
		if (newPts == nullptr) {
			return false;
		}

		bx::memCopy(newPts, arr->m_Pts, sizeof(Vec2d) * arr->m_Size);

		BX_ALIGNED_FREE(arr->m_Allocator, arr->m_Pts, 16);
		arr->m_Pts = newPts;
		arr->m_Capacity = n;
	}

	arr->m_Size = n;
	return true;
}

bool vec2dArrFromScalar(Vec2dArray* arr, const double* x, const double* y, uint32_t n)
{
	if (!vec2dArrResize(arr, n)) {
		return false;
	}

	Vec2d* pts = arr->m_Pts;
	for (uint32_t i = 0; i < n; ++i) {
		pts->x = *x++;
		pts->y = *y++;
		pts++;
	}

	return true;
}

bool vec2Arrd2f(Vec2fArray* dst, const Vec2dArray* src)
{
	const uint32_t n = src->m_Size;
	if (!vec2fArrResize(dst, n)) {
		return false;
	}

	Vec2f* dstPts = dst->m_Pts;
	const Vec2d* srcPts = src->m_Pts;
	for (uint32_t i = 0; i < n; ++i) {
		dstPts->x = (float)srcPts->x;
		dstPts->y = (float)srcPts->y;
		++dstPts;
		++srcPts;
	}

	return true;
}
}