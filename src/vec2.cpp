#include <jx/vec2.h>
#include <jx/sys.h>
#include <bx/bx.h>
#include <bx/allocator.h>
#include <bx/math.h>

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
	if (arr->m_Pts != nullptr) {
		BX_ALIGNED_FREE(arr->m_Allocator, arr->m_Pts, 16);
		arr->m_Pts = 0;
		arr->m_Size = 0;
		arr->m_Capacity = 0;
	}
}

bool vec2fArrResize(Vec2fArray* arr, uint32_t n)
{
	if (n > arr->m_Capacity) {
		const uint32_t oldCapacity = arr->m_Capacity;
		const uint32_t newCapacity_auto = oldCapacity != 0
			? (oldCapacity * 3) / 2
			: n
			;
		const uint32_t newCapacity = bx::max<uint32_t>(n, newCapacity_auto);

		Vec2f* newPts = (Vec2f*)BX_ALIGNED_ALLOC(arr->m_Allocator, sizeof(Vec2f) * newCapacity, 16);
		if (newPts == nullptr) {
			return false;
		}

		bx::memCopy(newPts, arr->m_Pts, sizeof(Vec2f) * arr->m_Size);

		BX_ALIGNED_FREE(arr->m_Allocator, arr->m_Pts, 16);
		arr->m_Pts = newPts;
		arr->m_Capacity = newCapacity;
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

uint32_t vec2fArrClosestPoint(const Vec2fArray* arr, const Vec2f& v, float* dist)
{
	float closestDistSqr = bx::kFloatLargest;
	uint32_t closestID = UINT32_MAX;

	const uint32_t n = arr->m_Size;
	for (uint32_t i = 0; i < n; ++i) {
		const float distSqr = vec2fDistanceSqr(arr->m_Pts[i], v);
		if (distSqr < closestDistSqr) {
			closestDistSqr = distSqr;
			closestID = i;
		}
	}

	if (dist) {
		*dist = closestDistSqr;
	}

	return closestID;
}

void vec2fArrRemovePoint(Vec2fArray* arr, uint32_t id)
{
	JX_CHECK(id < arr->m_Size, "Invalid point id");
	if (id != arr->m_Size - 1) {
		bx::memMove(&arr->m_Pts[id], &arr->m_Pts[id + 1], sizeof(Vec2f) * (arr->m_Size - id - 1));
	}
	arr->m_Size--;
}

bool vec2fArrCalcBoundingRect(const Vec2fArray* arr, Vec2f& minPt, Vec2f& maxPt)
{
	const uint32_t numPoints = arr->m_Size;
	if (numPoints == 0) {
		return false;
	}

	minPt = arr->m_Pts[0];
	maxPt = arr->m_Pts[0];
	for (uint32_t i = 1; i < numPoints; ++i) {
		const Vec2f& pt = arr->m_Pts[i];
		minPt.x = bx::min<float>(minPt.x, pt.x);
		minPt.y = bx::min<float>(minPt.y, pt.y);
		maxPt.x = bx::max<float>(maxPt.x, pt.x);
		maxPt.y = bx::max<float>(maxPt.y, pt.y);
	}

	return true;
}

bool vec2fArrPushBack(Vec2fArray* arr, const Vec2f& pt)
{
	if (arr->m_Size == arr->m_Capacity) {
		const uint32_t oldCapacity = arr->m_Capacity;
		const uint32_t newCapacity = oldCapacity + 32;

		jx::Vec2f* newPts = (jx::Vec2f*)BX_ALIGNED_ALLOC(arr->m_Allocator, sizeof(jx::Vec2f) * newCapacity, 16);
		if (!newPts) {
			return false;
		}

		bx::memCopy(newPts, arr->m_Pts, sizeof(jx::Vec2f) * oldCapacity);
		bx::memSet(&newPts[oldCapacity], 0, sizeof(jx::Vec2f) * (newCapacity - oldCapacity));

		BX_ALIGNED_FREE(arr->m_Allocator, arr->m_Pts, 16);
		arr->m_Pts = newPts;
		arr->m_Capacity = newCapacity;
	}

	arr->m_Pts[arr->m_Size] = pt;
	arr->m_Size++;

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
	if (arr->m_Pts != nullptr) {
		bx::AllocatorI* allocator = arr->m_Allocator;
		BX_ALIGNED_FREE(allocator, arr->m_Pts, 16);
		arr->m_Pts = 0;
		arr->m_Size = 0;
		arr->m_Capacity = 0;
	}
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

bool vec2dArrCalcBoundingRect(const Vec2dArray* arr, Vec2d& minPt, Vec2d& maxPt)
{
	const uint32_t numPoints = arr->m_Size;
	if (numPoints == 0) {
		return false;
	}

	minPt = arr->m_Pts[0];
	maxPt = arr->m_Pts[0];
	for (uint32_t i = 1; i < numPoints; ++i) {
		const Vec2d& pt = arr->m_Pts[i];
		minPt.x = bx::min<double>(minPt.x, pt.x);
		minPt.y = bx::min<double>(minPt.y, pt.y);
		maxPt.x = bx::max<double>(maxPt.x, pt.x);
		maxPt.y = bx::max<double>(maxPt.y, pt.y);
	}

	return true;
}
}
