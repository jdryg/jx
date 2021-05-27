#ifndef JX_MATRIX_H
#error "Must be included from jx/matrix.h"
#endif

namespace jx
{
inline void matfSet(Matrixf* mat, uint32_t row, uint32_t col, float val)
{
	mat->m_Elem[row + col * mat->m_NumRows] = val;
}

inline float matfGet(const Matrixf* mat, uint32_t row, uint32_t col)
{
	return mat->m_Elem[row + col * mat->m_NumRows];
}
}
