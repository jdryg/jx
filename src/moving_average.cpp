#include <jx/moving_average.h>
#include <jx/sys.h>
#include <bx/allocator.h>
#include <bx/math.h>

namespace jx
{
struct MovingAverage
{
	bx::AllocatorI* m_Allocator;
	float* m_Data;
	float m_Total;
	uint32_t m_Capacity;
	uint32_t m_Count;
	uint32_t m_InsertPos;
};

inline uint32_t alignSize(uint32_t sz, uint32_t alignment)
{
	JX_CHECK(bx::isPowerOf2<uint32_t>(alignment), "Invalid alignment value");
	const uint32_t mask = alignment - 1;
	return (sz & (~mask)) + ((sz & mask) != 0 ? alignment : 0);
}

MovingAverage* createMovingAverage(bx::AllocatorI* allocator, uint32_t n)
{
	const uint32_t totalMem = 0
		+ alignSize(sizeof(MovingAverage), 16)
		+ alignSize(sizeof(float) * n, 16);

	uint8_t* buffer = (uint8_t*)BX_ALIGNED_ALLOC(allocator, totalMem, 16);
	if (!buffer) {
		return nullptr;
	}

	bx::memSet(buffer, 0, totalMem);

	uint8_t* ptr = buffer;
	MovingAverage* ma = (MovingAverage*)ptr; ptr += alignSize(sizeof(MovingAverage), 16);
	ma->m_Data = (float*)ptr; ptr += alignSize(sizeof(float) * n, 16);

	ma->m_Allocator = allocator;
	ma->m_InsertPos = 0;
	ma->m_Count = 0;
	ma->m_Total = 0.0f;
	ma->m_Capacity = n;

	return ma;
}

void destroyMovingAverage(MovingAverage* ma)
{
	bx::AllocatorI* allocator = ma->m_Allocator;

	BX_ALIGNED_FREE(allocator, ma, 16);
}

float movAvgPush(MovingAverage* ma, float val)
{
	ma->m_Total -= ma->m_Data[ma->m_InsertPos];
	ma->m_Total += val;
	ma->m_Data[ma->m_InsertPos] = val;
	ma->m_InsertPos = (ma->m_InsertPos + 1) % ma->m_Capacity;

	ma->m_Count = bx::min<uint32_t>(ma->m_Count + 1, ma->m_Capacity);

	return movAvgGetAverage(ma);
}

float movAvgGetAverage(const MovingAverage* ma)
{
	return ma->m_Total / (float)ma->m_Count;
}

float movAvgGetStdDev(const MovingAverage* ma)
{
	if (ma->m_Count == 0) {
		return 0.0f;
	}

	const float avg = movAvgGetAverage(ma);
	float stdDev = 0.0f;
	const uint32_t n = ma->m_Count;
	for (uint32_t i = 0; i < n; ++i) {
		const float v = ma->m_Data[i];
		const float d = v - avg;

		stdDev += d * d;
	}

	return bx::sqrt(stdDev / ma->m_Count);
}

void movAvgGetBounds(const MovingAverage* ma, float* minVal, float* maxVal)
{
	float min = 0.0f, max = 0.0f;
	if (ma->m_Count != 0) {
		min = ma->m_Data[0];
		max = ma->m_Data[0];
		const uint32_t n = ma->m_Count;
		for (uint32_t i = 1; i < n; ++i) {
			const float v = ma->m_Data[i];
			min = bx::min<float>(min, v);
			max = bx::max<float>(max, v);
		}
	}

	if (minVal) {
		*minVal = min;
	}
	if (maxVal) {
		*maxVal = max;
	}
}
}
