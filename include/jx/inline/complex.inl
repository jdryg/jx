#ifndef JX_COMPLEX_H
#error "Must be included from jx/complex.h"
#endif

namespace jx
{
template<typename T>
static inline T logHypot(T a, T b)
{
	if (b == 0.0) {
		return log(abs(a));
	}

	if (a == 0.0) {
		return log(abs(b));
	}

	if (abs(a) < 3000.0f && abs(b) < 3000.0f) {
		return log(a * a + b * b) * 0.5f;
	}

	return log(a / cos(atan2(b, a)));
}

template<typename T>
inline Complex<T> complex(T re, T im)
{
	Complex<T> c;
	c.re = re;
	c.im = im;
	return c;
}

template<typename T>
inline Complex<T> cadd(const Complex<T>& a, const Complex<T>& b)
{
	Complex<T> c;
	c.re = a.re + b.re;
	c.im = a.im + b.im;
	return c;
}

template<typename T>
inline Complex<T> csub(const Complex<T>& a, const Complex<T>& b)
{
	Complex<T> c;
	c.re = a.re - b.re;
	c.im = a.im - b.im;
	return c;
}

template<typename T>
inline Complex<T> cmul(const Complex<T>& a, const Complex<T>& b)
{
	Complex<T> c;
	c.re = a.re * b.re - a.im * b.im;
	c.im = a.re * b.im + a.im * b.re;
	return c;
}

template<typename T>
inline Complex<T> cdiv(const Complex<T>& a, const Complex<T>& b)
{
	const Complex<T> bconj = cconj(b);
	const T denom = cmul(b, bconj).re;
	const Complex<T> numer = cmul(a, bconj);
	return cscale(numer, T(1.0) / denom);
}

template<typename T>
inline Complex<T> cscale(const Complex<T>& a, T s)
{
	Complex<T> c;
	c.re = a.re * s;
	c.im = a.im * s;
	return c;
}

template<typename T>
inline T cabs(const Complex<T>& a)
{
	return sqrt(a.re * a.re + a.im * a.im);
}

template<typename T>
inline T cabsSqr(const Complex<T>& a)
{
	return a.re * a.re + a.im * a.im;
}

template<typename T>
inline Complex<T> cconj(const Complex<T>& a)
{
	Complex<T> c;
	c.re = a.re;
	c.im = -a.im;
	return c;
}

template<typename T>
inline Complex<T> cexp(const Complex<T>& a)
{
	const T exp_re = exp(a.re);
	const T cos_im = cos(a.im);
	const T sin_im = sin(a.im);

	Complex<T> c;
	c.re = exp_re * cos_im;
	c.im = exp_re * sin_im;
	return c;
}

// sin(a + bi) = sina * coshb + icosa * sinhb
template<typename T>
inline Complex<T> csin(const Complex<T>& c)
{
	Complex<T> res;
	res.re = sin(c.re) * cosh(c.im);
	res.im = cos(c.re) * sinh(c.im);
	return res;
}

// cos(a + bi) = cosa * coshb − isina * sinhb
template<typename T>
inline Complex<T> ccos(const Complex<T>& c)
{
	Complex<T> res;
	res.re = cos(c.re) * cosh(c.im);
	res.im = -sin(c.re) * sinh(c.im);
	return res;
}

// asin(c) = -i * log(ci + sqrt(1 - c^2))
template<typename T>
inline Complex<T> carcsin(const Complex<T>& c)
{
	const T a = c.re;
	const T b = c.im;

	const Complex<T> t1 = csqrt(complex(b * b - a * a + 1.0, -2.0 * a * b));
	const Complex<T> t2 = clog(complex(t1.re - b, t1.im + a));

	Complex<T> res;
	res.re = t2.im;
	res.im = -t2.re;
	return res;
}

template<typename T>
inline Complex<T> clog(const Complex<T>& c)
{
	const T a = c.re;
	const T b = c.im;

	Complex<T> res;
	res.re = logHypot(a, b);
	res.im = atan2(b, a);
	return res;
}

template<typename T>
inline Complex<T> cinv(const Complex<T>& a)
{
	return cscale(cconj(a), 1.0 / cabsSqr(a));
}

template<typename T>
inline Complex<T> csqrt(const Complex<T>& a)
{
	const T abs_a = cabs(a);

	Complex<T> c;
	c.re = sqrt(0.5 * (abs_a + a.re));
	c.im = sqrt(0.5 * (abs_a - a.re));
	return c;
}

template<typename T>
inline ComplexMatrix<T> complexMatrix(const Complex<T>& a, const Complex<T>& b, const Complex<T>& c, const Complex<T>& d)
{
	ComplexMatrix<T> res;
	res.m_Elem[0] = a;
	res.m_Elem[1] = b;
	res.m_Elem[2] = c;
	res.m_Elem[3] = d;
	return res;
}

// Using cXXX functions    : 4.64ms/iter (coh_tmm: 22.8%, cmmul: 19.9%)
// Using inlined cXXX funcs: 4.01ms/iter (coh_tmm: 27.4%, cmmul: 7.9%)
// Using SSE               : 3.90ms/iter (coh_tmm: 29.3%, cmmul: 3.7%)
template<typename T>
inline ComplexMatrix<T> cmmul(const ComplexMatrix<T>& a, const ComplexMatrix<T>& b)
{
	const Complex<T>& a0 = a.m_Elem[0];
	const Complex<T>& a1 = a.m_Elem[1];
	const Complex<T>& a2 = a.m_Elem[2];
	const Complex<T>& a3 = a.m_Elem[3];
	const Complex<T>& b0 = b.m_Elem[0];
	const Complex<T>& b1 = b.m_Elem[1];
	const Complex<T>& b2 = b.m_Elem[2];
	const Complex<T>& b3 = b.m_Elem[3];

	const Complex<T> a0b0 = complex(a0.re * b0.re - a0.im * b0.im, a0.re * b0.im + a0.im * b0.re);
	const Complex<T> a0b1 = complex(a0.re * b1.re - a0.im * b1.im, a0.re * b1.im + a0.im * b1.re);
	const Complex<T> a2b0 = complex(a2.re * b0.re - a2.im * b0.im, a2.re * b0.im + a2.im * b0.re);
	const Complex<T> a2b1 = complex(a2.re * b1.re - a2.im * b1.im, a2.re * b1.im + a2.im * b1.re);
	const Complex<T> a1b2 = complex(a1.re * b2.re - a1.im * b2.im, a1.re * b2.im + a1.im * b2.re);
	const Complex<T> a1b3 = complex(a1.re * b3.re - a1.im * b3.im, a1.re * b3.im + a1.im * b3.re);
	const Complex<T> a3b2 = complex(a3.re * b2.re - a3.im * b2.im, a3.re * b2.im + a3.im * b2.re);
	const Complex<T> a3b3 = complex(a3.re * b3.re - a3.im * b3.im, a3.re * b3.im + a3.im * b3.re);

	const Complex<T> c00 = complex(a0b0.re + a1b2.re, a0b0.im + a1b2.im);
	const Complex<T> c01 = complex(a0b1.re + a1b3.re, a0b1.im + a1b3.im);
	const Complex<T> c10 = complex(a2b0.re + a3b2.re, a2b0.im + a3b2.im);
	const Complex<T> c11 = complex(a2b1.re + a3b3.re, a2b1.im + a3b3.im);

	return complexMatrix(c00, c01, c10, c11);
}

template<typename T>
inline ComplexMatrix<T> cmscale(const ComplexMatrix<T>& c, const Complex<T>& scale)
{
	return complexMatrix(cmul(c.m_Elem[0], scale), cmul(c.m_Elem[1], scale), cmul(c.m_Elem[2], scale), cmul(c.m_Elem[3], scale));
}
}
