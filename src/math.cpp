#include <jx/math.h>
#include <bx/allocator.h>

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

template<typename T>
void linspace(T start, T end, T* x, uint32_t n)
{
	const T d = (end - start) / (n - 1);
	T t = start;
	for (uint32_t i = 0; i < n; ++i) {
		x[i] = t;
		t += d;
	}
}

void linspacef(float start, float end, float* x, uint32_t n)
{
	linspace<float>(start, end, x, n);
}

void linspaced(double start, double end, double* x, uint32_t n)
{
	linspace<double>(start, end, x, n);
}

template<typename T, uint32_t DIM>
static inline uint32_t locate1(const T* arr, uint32_t n, T val)
{
#define ELEM(i) arr[(i) * DIM]

	if (n < 2) {
		return ~0u;
	}

	const bool asc = ELEM(n - 1) >= ELEM(0);
#if 0
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
#else
	if ((asc && (ELEM(0) >= val)) || (!asc && (ELEM(0) <= val))) {
		return 0;
	} else if ((asc && (ELEM(n - 1) <= val)) || (!asc && (ELEM(n - 1) >= val))) {
		return n - 2;
	}

	uint32_t l = 0;
	uint32_t r = n;
	while (l < r) {
		const uint32_t m = (l + r) >> 1;
		if ((val >= ELEM(m)) == asc) {
			l = m + 1;
		} else {
			r = m;
		}
	}

	return l - 1; // l holds the number of elements less than val.
#endif
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
		yq[i] = bx::lerp(y[j], y[j + 1], t);
	}
}

template<typename T>
static bool cubicSplineInterp(const T* x, const T* y, uint32_t n, const T* resX, T* resY, uint32_t resN, bx::AllocatorI* allocator)
{
	const uint32_t numSplines = n - 1;

	const uint32_t totalMemory = sizeof(T) * (0
		+ numSplines // h
		+ numSplines // A
		+ n // l
		+ n // u
		+ n // z
		+ n // a
		+ n // b
		+ n // c
		+ n // d
		);

	T* buf = (T*)BX_ALLOC(allocator, totalMemory);
	if (!buf) {
		return false;
	}

	T* ptr = buf;
	T* h = ptr; ptr += numSplines;
	T* A = ptr; ptr += numSplines;
	T* l = ptr; ptr += n;
	T* u = ptr; ptr += n;
	T* z = ptr; ptr += n;
	T* a = ptr; ptr += n;
	T* b = ptr; ptr += n;
	T* c = ptr; ptr += n;
	T* d = ptr; ptr += n;

	for (uint32_t i = 0; i < numSplines; ++i) {
		h[i] = x[i + 1] - x[i];
	}

	for (uint32_t i = 1; i < numSplines; ++i) {
		A[i] = 0
			+ T(3.0) * (y[i + 1] - y[i + 0]) / h[i + 0]
			- T(3.0) * (y[i + 0] - y[i - 1]) / h[i - 1];
	}

	l[0] = T(1.0);
	u[0] = T(0.0);
	z[0] = T(0.0);

	for (uint32_t i = 1; i < numSplines; ++i) {
		l[i] = T(2.0) * (x[i + 1] - x[i - 1]) - h[i - 1] * u[i - 1];
		u[i] = h[i] / l[i];
		z[i] = (A[i] - h[i - 1] * z[i - 1]) / l[i];
	}

	l[numSplines] = T(1.0);
	z[numSplines] = T(0.0);

	a[numSplines] = y[numSplines];
	c[numSplines] = T(0.0);
	for (int32_t j = (int32_t)(numSplines - 1); j >= 0; --j) {
		c[j] = z[j] - u[j] * c[j + 1];
		b[j] = (y[j + 1] - y[j]) / h[j] - h[j] * (c[j + 1] + T(2.0) * c[j]) / T(3.0);
		d[j] = (c[j + 1] - c[j]) / (T(3.0) * h[j]);
		a[j] = y[j];
	}

	for (uint32_t i = 0; i < resN; ++i) {
		const uint32_t splineID = locate1<T, 1>(x, n, resX[i]);
		const T rx = resX[i] - x[splineID];
		resY[i] = a[splineID] + rx * (b[splineID] + rx * (c[splineID] + rx * d[splineID]));
	}

	BX_FREE(allocator, buf);

	return true;
}

bool cubicSplineInterp1f(const float* x, const float* y, uint32_t n, const float* resX, float* resY, uint32_t resN, bx::AllocatorI* allocator)
{
	return cubicSplineInterp<float>(x, y, n, resX, resY, resN, allocator);
}
}
