#include <jx/matrix.h>
#include <jx/sys.h>
#include <bx/allocator.h>
#include <bx/math.h>

namespace jx
{
bool matfInit(Matrixf* mat, uint32_t nr, uint32_t nc, bx::AllocatorI* allocator)
{
	mat->m_Elem = (float*)BX_ALLOC(allocator, sizeof(float) * nr * nc);
	if (!mat->m_Elem) {
		return false;
	}

	mat->m_NumRows = nr;
	mat->m_NumCols = nc;

	return true;
}

void matfDestroy(Matrixf* mat, bx::AllocatorI* allocator)
{
	BX_FREE(allocator, mat->m_Elem);
	mat->m_Elem = nullptr;
	mat->m_NumRows = 0;
	mat->m_NumCols = 0;
}

void matfSwapRows(Matrixf* mat, uint32_t i, uint32_t j)
{
	const uint32_t nc = mat->m_NumCols;
	for (uint32_t col = 0; col < nc; ++col) {
		const uint32_t offset = col * nc;
		bx::swap(mat->m_Elem[i + offset], mat->m_Elem[j + offset]);
	}
}

// https://en.wikipedia.org/wiki/LU_decomposition#C_code_example
// INPUT: A - NxN matrix
// OUTPUT: Matrix A is changed, it contains a copy of both matrices L-E and U as A=(L-E)+U such that P*A=L*U.
//        The permutation matrix is not stored as a matrix, but in an integer vector P of size N
//        containing column indexes where the permutation matrix has "1". numPivots = S+N,
//        where S is the number of row exchanges needed for determinant computation, det(P)=(-1)^S
bool matfLUPDecompose(jx::Matrixf* A, uint32_t* P, uint32_t* numPivots)
{
	const uint32_t n = A->m_NumRows;
	JX_CHECK(A->m_NumCols == n, "matfLUPDecompose() requires a square matrix");

	for (uint32_t i = 0; i < n; i++) {
		P[i] = i; // Unit permutation matrix
	}

	uint32_t np = n;
	for (uint32_t i = 0; i < n; i++) {
		float maxA = 0.0f;
		uint32_t imax = i;

		for (uint32_t k = i; k < n; k++) {
			const float absA = bx::abs(jx::matfGet(A, k, i));
			if (absA > maxA) {
				maxA = absA;
				imax = k;
			}
		}

		if (maxA < bx::kFloatMin) {
			return false; // failure, matrix is degenerate
		}

		if (imax != i) {
			// pivoting P
			bx::swap(P[i], P[imax]);

			// pivoting rows of A
			matfSwapRows(A, i, imax);

			// counting pivots starting from N (for determinant)
			np++;
		}

		for (uint32_t j = i + 1; j < n; j++) {
			jx::matfSet(A, j, i, jx::matfGet(A, j, i) / jx::matfGet(A, i, i));

			for (uint32_t k = i + 1; k < n; k++) {
				jx::matfSet(A, j, k, jx::matfGet(A, j, k) - (jx::matfGet(A, j, i) * jx::matfGet(A, i, k)));
			}
		}
	}

	if (numPivots) {
		*numPivots = np;
	}

	return true;  // decomposition done 
}

// INPUT: A,P filled in LUPDecompose; b - rhs vector; N - dimension
// OUTPUT: x - solution vector of A*x=b
void matfLUPSolve(const jx::Matrixf* A, const uint32_t* P, const float* b, float* x)
{
	const uint32_t n = A->m_NumRows;
	for (uint32_t i = 0; i < n; i++) {
		x[i] = b[P[i]];

		for (uint32_t k = 0; k < i; k++) {
			const float Aik = jx::matfGet(A, i, k);
			x[i] -= Aik * x[k];
		}
	}

	for (int32_t i = (int32_t)(n - 1); i >= 0; i--) {
		for (uint32_t k = i + 1; k < n; k++) {
			const float Aik = jx::matfGet(A, i, k);
			x[i] -= Aik * x[k];
		}

		x[i] /= jx::matfGet(A, i, i);
	}
}
} // namespace jx
