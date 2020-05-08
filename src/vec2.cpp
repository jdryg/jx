#include <jx/vec2.h>
#include <jx/sys.h>
#include <bx/bx.h>
#include <bx/allocator.h>

namespace jx
{
bool vec2ArrInit(Vec2Array* arr, uint32_t n, bx::AllocatorI* allocator)
{
	bx::memSet(arr, 0, sizeof(Vec2Array));
	arr->m_Allocator = allocator != nullptr ? allocator : getGlobalAllocator();

	return n != 0 ? vec2ArrResize(arr, n) : true;
}

void vec2ArrDestroy(Vec2Array* arr)
{
	bx::AllocatorI* allocator = arr->m_Allocator;
	BX_ALIGNED_FREE(allocator, arr->m_Pts, 16);
	arr->m_Pts = 0;
	arr->m_Size = 0;
	arr->m_Capacity = 0;
}

bool vec2ArrResize(Vec2Array* arr, uint32_t n)
{
	if (n > arr->m_Capacity) {
		Vec2* newPts = (Vec2*)BX_ALIGNED_ALLOC(arr->m_Allocator, sizeof(Vec2) * n, 16);
		if (newPts == nullptr) {
			return false;
		}

		bx::memCopy(newPts, arr->m_Pts, sizeof(Vec2) * arr->m_Size);

		BX_ALIGNED_FREE(arr->m_Allocator, arr->m_Pts, 16);
		arr->m_Pts = newPts;
		arr->m_Capacity = n;
	}

	arr->m_Size = n;
	return true;
}
}
