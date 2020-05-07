#include <jx/math.h>

namespace jx
{
bool closestPointOnLineSegment(float x, float y, float sx, float sy, float ex, float ey, float thickness, float* t)
{
	const float esx = ex - sx;
	const float esy = ey - sy;
	const float psx = x - sx;
	const float psy = y - sy;

	const float segmentLengthSqr = esx * esx + esy * esy;
	if (segmentLengthSqr < 1e-5f) {
		// s == e
		if (t) {
			*t = 0.0f;
		}

		return (psx * psx + psy * psy) <= thickness * thickness;
	}

	const float dist = (psx * esx + psy * esy) / segmentLengthSqr;
	if (dist < 0.0f) {
		if (t) {
			*t = 0.0f;
		}

		return (psx * psx + psy * psy) <= thickness * thickness;
	} else if (dist > 1.0f) {
		const float pex = x - ex;
		const float pey = y - ey;
		if (t) {
			*t = 1.0f;
		}

		return (pex * pex + pey * pey) <= thickness * thickness;
	}

	if (t) {
		*t = dist;
	}

	const float pprojx = psx - esx * dist;
	const float pprojy = psy - esy * dist;
	return (pprojx * pprojx + pprojy * pprojy) <= thickness * thickness;
}

void linspacef(float start, float end, float* x, uint32_t n)
{
	const float d = (end - start) / (n - 1);
	float t = start;
	for (uint32_t i = 0; i < n; ++i) {
		x[i] = t;
		t += d;
	}
}

template<typename T, uint32_t DIM>
static inline uint32_t locate1(const T* arr, uint32_t n, T val)
{
#define ELEM(i) arr[(i) * DIM]

	if (n < 2) {
		return ~0u;
	}

	const bool asc = ELEM(n - 1) >= ELEM(0);
	uint32_t l = 0;
	uint32_t u = n - 1;
	while (u - l > 1) {
		const uint32_t m = (u + l) >> 1;
		if ((val >= ELEM(m)) == asc) {
			l = m;
		} else {
			u = m;
		}
	}

	return l < n ? l : n - 1;
#undef ELEM
}

uint32_t locate1f(const float* arr, uint32_t n, float val)
{
	return locate1<float, 1>(arr, n, val);
}

uint32_t locate1d(const double* arr, uint32_t n, double val)
{
	return locate1<double, 1>(arr, n, val);
}

uint32_t locate2f(const float* arr, uint32_t n, float val)
{
	return locate1<float, 2>(arr, n, val);
}

uint32_t locate2d(const double* arr, uint32_t n, double val)
{
	return locate1<double, 2>(arr, n, val);
}

void interp1f(const float* x, const float* y, uint32_t n, const float* xq, float* yq, uint32_t nq)
{
	for (uint32_t i = 0; i < nq; ++i) {
		const float q = xq[i];
		const uint32_t j = jx::locate1f(x, n, q);

		const float t = (q - x[j]) / (x[j + 1] - x[j]);
		yq[i] = y[j] + t * (y[j + 1] - y[j]);
	}
}
}
