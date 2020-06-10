#ifndef JX_COMPLEX_H
#define JX_COMPLEX_H

#include <math.h>

namespace jx
{
template<typename T>
struct Complex
{
	T re;
	T im;
};

// [0 1]
// [2 3]
template<typename T>
struct ComplexMatrix
{
	Complex<T> m_Elem[4];
};

typedef Complex<float>  Complexf;
typedef Complex<double> Complexd;

template<typename T> Complex<T> complex(T re, T im);
template<typename T> Complex<T> cadd(const Complex<T>& a, const Complex<T>& b);
template<typename T> Complex<T> csub(const Complex<T>& a, const Complex<T>& b);
template<typename T> Complex<T> cmul(const Complex<T>& a, const Complex<T>& b);
template<typename T> Complex<T> cdiv(const Complex<T>& a, const Complex<T>& b);
template<typename T> Complex<T> cscale(const Complex<T>& a, T s);
template<typename T> T cabs(const Complex<T>& a);
template<typename T> T cabsSqr(const Complex<T>& a);
template<typename T> Complex<T> cconj(const Complex<T>& a);
template<typename T> Complex<T> cexp(const Complex<T>& a);
template<typename T> Complex<T> cinv(const Complex<T>& a);
template<typename T> Complex<T> csqrt(const Complex<T>& a);
template<typename T> Complex<T> csin(const Complex<T>& c);
template<typename T> Complex<T> ccos(const Complex<T>& c);
template<typename T> Complex<T> carcsin(const Complex<T>& c);
template<typename T> Complex<T> clog(const Complex<T>& c);

template<typename T> ComplexMatrix<T> complexMatrix(const Complex<T>& a, const Complex<T>& b, const Complex<T>& c, const Complex<T>& d);
template<typename T> ComplexMatrix<T> cmscale(const ComplexMatrix<T>& a, const Complex<T>& b);
template<typename T> ComplexMatrix<T> cmmul(const ComplexMatrix<T>& a, const ComplexMatrix<T>& b);
}

#include "inline/complex.inl"

#endif
