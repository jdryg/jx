#ifndef JX_MATH_H
#define JX_MATH_H

#include <stdint.h>
#include <bx/math.h>

namespace bx
{
struct AllocatorI;
}

namespace jx
{
uint32_t nextPowerOf2(uint32_t v);
uint32_t log2ui(uint32_t v);
uint32_t bitcount(uint32_t v);

float snapTo(float x, float stepSize);

uint32_t solveQuadratic(const float* coefs, float* res);

bool closestPointOnLineSegment(float x, float y, float sx, float sy, float ex, float ey, float thickness, float* t);

void linspacef(float start, float end, float* x, uint32_t n);
void linspaced(double start, double end, double* x, uint32_t n);

// Given an array arr, and given a value val, returns a value j such that val is between arr[j]
// and arr[j + 1]. arr must be monotonic, either increasing or decreasing. j = UINT32_MAX is returned
// on error (i.e. if n < 2). 
// If val is out of range the closest j is returned.
// E.g. for a ascending array arr, 
// - if val < arr[0] then j = 0, so that linear interpolation between arr[0] and arr[1] gives a factor
//   less than 0
// - if val > arr[n-1] the j = n-2, so that linear interpolation between arr[n-2] and arr[n-1] gives
//   a factor greater than 1.
uint32_t locate1f(const float* arr, uint32_t n, float val);
uint32_t locate1d(const double* arr, uint32_t n, double val);

// Same as locate1 but arr is 2-dimensional. val corresponds to the first dimension
uint32_t locate2f(const float* arr, uint32_t n, float val);
uint32_t locate2d(const double* arr, uint32_t n, double val);

void interp1f(const float* x, const float* y, uint32_t n, const float* xq, float* yq, uint32_t nq);
void interp1d(const double* x, const double* y, uint32_t n, const double* xq, double* yq, uint32_t nq);

bool cubicSplineInterp1f(const float* x, const float* y, uint32_t n, const float* resX, float* resY, uint32_t resN, bx::AllocatorI* allocator);
}

#include "inline/math.inl"

#endif
