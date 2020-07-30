#ifndef JX_SORT_H_
#define JX_SORT_H_

#include <stdint.h>

namespace jx
{
typedef int32_t ComparisonFunc(const void* elem1, const void* elem2, void* ctx);
typedef void SwapFunc(void* arr, int32_t i, int32_t j, void* ctx);

// QuickSort with context + custom swap function. Useful for (e.g.) sorting objects 
// in SoA form or an array of handles based on object's name or any other property.
void quickSort(void* arr, uint32_t elemSize, uint32_t numElements, ComparisonFunc cmpFunc, SwapFunc swapFunc, void* ctx);
}

#endif
