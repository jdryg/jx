#ifndef JX_MATRIX_H
#define JX_MATRIX_H

#include <stdint.h>

namespace bx
{
struct AllocatorI;
}

namespace jx
{
struct Matrixf
{
	float* m_Elem;
	uint32_t m_NumRows;
	uint32_t m_NumCols;
};

bool matfInit(Matrixf* mat, uint32_t nr, uint32_t nc, bx::AllocatorI* allocator);
void matfDestroy(Matrixf* mat, bx::AllocatorI* allocator);

void matfSwapRows(Matrixf* mat, uint32_t i, uint32_t j);
void matfSet(Matrixf* mat, uint32_t row, uint32_t col, float val);
float matfGet(const Matrixf* mat, uint32_t row, uint32_t col);

bool matfLUPDecompose(jx::Matrixf* A, uint32_t* P, uint32_t* numPivots);
void matfLUPSolve(const jx::Matrixf* A, const uint32_t* P, const float* b, float* x);
}

#include "inline/matrix.inl"

#endif // JX_MATRIX_H
