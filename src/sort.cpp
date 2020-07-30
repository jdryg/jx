#include <jx/sort.h>
#include <bx/bx.h>

namespace jx
{
static void quickSort_r(void* arr, uint32_t elemSize, int32_t low, int32_t high, ComparisonFunc cmpFunc, SwapFunc swapFunc, void* ctx);
static int32_t quickSortPartition(void* arr, uint32_t elemSize, int32_t low, int32_t high, ComparisonFunc cmpFunc, SwapFunc swapFunc, void* ctx);

// Recursive Quicksort which always selects last element as pivot (bad for already partially sorted arrays?)
void quickSort(void* arr, uint32_t elemSize, uint32_t numElements, ComparisonFunc cmpFunc, SwapFunc swapFunc, void* ctx)
{
	quickSort_r(arr, elemSize, 0, numElements - 1, cmpFunc, swapFunc, ctx);
}

static void quickSort_r(void* arr, uint32_t elemSize, int32_t low, int32_t high, ComparisonFunc cmpFunc, SwapFunc swapFunc, void* ctx)
{
	if (low < high) {
		const int32_t partitionIndex = quickSortPartition(arr, elemSize, low, high, cmpFunc, swapFunc, ctx);

		quickSort_r(arr, elemSize, low, partitionIndex - 1, cmpFunc, swapFunc, ctx);
		quickSort_r(arr, elemSize, partitionIndex + 1, high, cmpFunc, swapFunc, ctx);
	}
}

static int32_t quickSortPartition(void* arr, uint32_t elemSize, int32_t low, int32_t high, ComparisonFunc cmpFunc, SwapFunc swapFunc, void* ctx)
{
	const int32_t pivotID = high;
	const uint8_t* elemPivot = (uint8_t*)arr + elemSize * pivotID;

	int32_t i = low;
	for (int32_t j = low; j < high; ++j) {
		const uint8_t* elemj = (uint8_t*)arr + elemSize * j;
		if (cmpFunc(elemj, elemPivot, ctx) < 0) {
			// Swap i and j elements
			if (i != j) {
				swapFunc(arr, i, j, ctx);
			}
			i++;
		}
	}

	if (i != high) {
		swapFunc(arr, i, high, ctx);
	}

	return i;
}
}
