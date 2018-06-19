#include <jx/rect.h>
#include <jx/sys.h>
#include <jx/bitset.h>

#if JX_CONFIG_MATH_SIMD
#include <xmmintrin.h>
#include <immintrin.h>
#endif

namespace jx
{
#if JX_CONFIG_MATH_SIMD
// Maps a 4-bit mask obtained from _mm_movemask_ps() to visibility result
// NOTE: Mask bits are set if rect is culled.
static const uint32_t s_MaskToVisibility4[16] = {
	0x01010101, // { 0, 0, 0, 0 }
	0x01010100, // { 0, 0, 0, 1 }
	0x01010001, // { 0, 0, 1, 0 }
	0x01010000, // { 0, 0, 1, 1 }
	0x01000101, // { 0, 1, 0, 0 }
	0x01000100, // { 0, 1, 0, 1 }
	0x01000001, // { 0, 1, 1, 0 }
	0x01000000, // { 0, 1, 1, 1 }
	0x00010101, // { 1, 0, 0, 0 }
	0x00010100, // { 1, 0, 0, 1 }
	0x00010001, // { 1, 0, 1, 0 }
	0x00010000, // { 1, 0, 1, 1 }
	0x00000101, // { 1, 1, 0, 0 }
	0x00000100, // { 1, 1, 0, 1 }
	0x00000001, // { 1, 1, 1, 0 }
	0x00000000, // { 1, 1, 1, 1 }
};

void rectAoSToSoA(const Rect* aos, uint32_t numRects, RectSoA* soa)
{
	const float* src = (float*)aos;
	float* dstMinX = soa->m_MinX;
	float* dstMinY = soa->m_MinY;
	float* dstMaxX = soa->m_MaxX;
	float* dstMaxY = soa->m_MaxY;

	const uint32_t numIter = numRects >> 3; // 8 rects/iter
	for (uint32_t i = 0; i < numIter; ++i) {
		const __m128 r0 = _mm_load_ps(src + 0);
		const __m128 r1 = _mm_load_ps(src + 4);
		const __m128 r2 = _mm_load_ps(src + 8);
		const __m128 r3 = _mm_load_ps(src + 12);
		const __m128 r4 = _mm_load_ps(src + 16);
		const __m128 r5 = _mm_load_ps(src + 20);
		const __m128 r6 = _mm_load_ps(src + 24);
		const __m128 r7 = _mm_load_ps(src + 28);

		const __m128 r01_min = _mm_movelh_ps(r0, r1);
		const __m128 r01_max = _mm_movehl_ps(r1, r0);
		const __m128 r23_min = _mm_movelh_ps(r2, r3);
		const __m128 r23_max = _mm_movehl_ps(r3, r2);
		const __m128 r45_min = _mm_movelh_ps(r4, r5);
		const __m128 r45_max = _mm_movehl_ps(r5, r4);
		const __m128 r67_min = _mm_movelh_ps(r6, r7);
		const __m128 r67_max = _mm_movehl_ps(r7, r6);

		const __m128 minx0123 = _mm_shuffle_ps(r01_min, r23_min, _MM_SHUFFLE(2, 0, 2, 0));
		const __m128 miny0123 = _mm_shuffle_ps(r01_min, r23_min, _MM_SHUFFLE(3, 1, 3, 1));
		const __m128 maxx0123 = _mm_shuffle_ps(r01_max, r23_max, _MM_SHUFFLE(2, 0, 2, 0));
		const __m128 maxy0123 = _mm_shuffle_ps(r01_max, r23_max, _MM_SHUFFLE(3, 1, 3, 1));
		const __m128 minx4567 = _mm_shuffle_ps(r45_min, r67_min, _MM_SHUFFLE(2, 0, 2, 0));
		const __m128 miny4567 = _mm_shuffle_ps(r45_min, r67_min, _MM_SHUFFLE(3, 1, 3, 1));
		const __m128 maxx4567 = _mm_shuffle_ps(r45_max, r67_max, _MM_SHUFFLE(2, 0, 2, 0));
		const __m128 maxy4567 = _mm_shuffle_ps(r45_max, r67_max, _MM_SHUFFLE(3, 1, 3, 1));

		_mm_store_ps(dstMinX + 0, minx0123);
		_mm_store_ps(dstMinY + 0, miny0123);
		_mm_store_ps(dstMaxX + 0, maxx0123);
		_mm_store_ps(dstMaxY + 0, maxy0123);
		_mm_store_ps(dstMinX + 4, minx4567);
		_mm_store_ps(dstMinY + 4, miny4567);
		_mm_store_ps(dstMaxX + 4, maxx4567);
		_mm_store_ps(dstMaxY + 4, maxy4567);

		src += 32;
		dstMinX += 8;
		dstMinY += 8;
		dstMaxX += 8;
		dstMaxY += 8;
	}

	uint32_t rem = numRects & 7;
	if (rem >= 4) {
		const __m128 r0 = _mm_load_ps(src + 0);
		const __m128 r1 = _mm_load_ps(src + 4);
		const __m128 r2 = _mm_load_ps(src + 8);
		const __m128 r3 = _mm_load_ps(src + 12);

		const __m128 r01_min = _mm_movelh_ps(r0, r1);
		const __m128 r01_max = _mm_movehl_ps(r1, r0);
		const __m128 r23_min = _mm_movelh_ps(r2, r3);
		const __m128 r23_max = _mm_movehl_ps(r3, r2);

		const __m128 minx0123 = _mm_shuffle_ps(r01_min, r23_min, _MM_SHUFFLE(2, 0, 2, 0));
		const __m128 miny0123 = _mm_shuffle_ps(r01_min, r23_min, _MM_SHUFFLE(3, 1, 3, 1));
		const __m128 maxx0123 = _mm_shuffle_ps(r01_max, r23_max, _MM_SHUFFLE(2, 0, 2, 0));
		const __m128 maxy0123 = _mm_shuffle_ps(r01_max, r23_max, _MM_SHUFFLE(3, 1, 3, 1));

		_mm_store_ps(dstMinX, minx0123);
		_mm_store_ps(dstMinY, miny0123);
		_mm_store_ps(dstMaxX, maxx0123);
		_mm_store_ps(dstMaxY, maxy0123);

		src += 16;
		dstMinX += 4;
		dstMinY += 4;
		dstMaxX += 4;
		dstMaxY += 4;

		rem -= 4;
	}

	if (rem >= 2) {
		const __m128 r0 = _mm_load_ps(src + 0);
		const __m128 r1 = _mm_load_ps(src + 4);

		const __m128 r01_min = _mm_movelh_ps(r0, r1);
		const __m128 r01_max = _mm_movehl_ps(r1, r0);

		const __m128 minx01_miny01 = _mm_shuffle_ps(r01_min, r01_min, _MM_SHUFFLE(3, 1, 2, 0));
		const __m128 maxx01_maxy01 = _mm_shuffle_ps(r01_max, r01_max, _MM_SHUFFLE(3, 1, 2, 0));

		_mm_storel_pi((__m64*)dstMinX, minx01_miny01);
		_mm_storeh_pi((__m64*)dstMinY, minx01_miny01);
		_mm_storel_pi((__m64*)dstMaxX, maxx01_maxy01);
		_mm_storeh_pi((__m64*)dstMaxY, maxx01_maxy01);

		src += 8;
		dstMinX += 2;
		dstMinY += 2;
		dstMaxX += 2;
		dstMaxY += 2;

		rem -= 2;
	}

	if (rem) {
		*dstMinX = src[0];
		*dstMinY = src[1];
		*dstMaxX = src[2];
		*dstMaxY = src[3];
	}
}

void rectSoAToAoS(const RectSoA* soa, uint32_t numRects, Rect* aos)
{
	const float* srcMinX = soa->m_MinX;
	const float* srcMinY = soa->m_MinY;
	const float* srcMaxX = soa->m_MaxX;
	const float* srcMaxY = soa->m_MaxY;
	float* dst = (float*)aos;

	const uint32_t numIter = numRects >> 3; // 8 rects/iter
	for (uint32_t i = 0; i < numIter; ++i) {
		const __m128 minx0123 = _mm_load_ps(srcMinX + 0);
		const __m128 miny0123 = _mm_load_ps(srcMinY + 0);
		const __m128 maxx0123 = _mm_load_ps(srcMaxX + 0);
		const __m128 maxy0123 = _mm_load_ps(srcMaxY + 0);
		const __m128 minx4567 = _mm_load_ps(srcMinX + 4);
		const __m128 miny4567 = _mm_load_ps(srcMinY + 4);
		const __m128 maxx4567 = _mm_load_ps(srcMaxX + 4);
		const __m128 maxy4567 = _mm_load_ps(srcMaxY + 4);

		const __m128 minx01_miny01 = _mm_movelh_ps(minx0123, miny0123);
		const __m128 minx23_miny23 = _mm_movehl_ps(miny0123, minx0123);
		const __m128 maxx01_maxy01 = _mm_movelh_ps(maxx0123, maxy0123);
		const __m128 maxx23_maxy23 = _mm_movehl_ps(maxy0123, maxx0123);
		const __m128 minx45_miny45 = _mm_movelh_ps(minx4567, miny4567);
		const __m128 minx67_miny67 = _mm_movehl_ps(miny4567, minx4567);
		const __m128 maxx45_maxy45 = _mm_movelh_ps(maxx4567, maxy4567);
		const __m128 maxx67_maxy67 = _mm_movehl_ps(maxy4567, maxx4567);

		const __m128 r0 = _mm_shuffle_ps(minx01_miny01, maxx01_maxy01, _MM_SHUFFLE(2, 0, 2, 0));
		const __m128 r1 = _mm_shuffle_ps(minx01_miny01, maxx01_maxy01, _MM_SHUFFLE(3, 1, 3, 1));
		const __m128 r2 = _mm_shuffle_ps(minx23_miny23, maxx23_maxy23, _MM_SHUFFLE(2, 0, 2, 0));
		const __m128 r3 = _mm_shuffle_ps(minx23_miny23, maxx23_maxy23, _MM_SHUFFLE(3, 1, 3, 1));
		const __m128 r4 = _mm_shuffle_ps(minx45_miny45, maxx45_maxy45, _MM_SHUFFLE(2, 0, 2, 0));
		const __m128 r5 = _mm_shuffle_ps(minx45_miny45, maxx45_maxy45, _MM_SHUFFLE(3, 1, 3, 1));
		const __m128 r6 = _mm_shuffle_ps(minx67_miny67, maxx67_maxy67, _MM_SHUFFLE(2, 0, 2, 0));
		const __m128 r7 = _mm_shuffle_ps(minx67_miny67, maxx67_maxy67, _MM_SHUFFLE(3, 1, 3, 1));

		_mm_store_ps(dst + 0, r0);
		_mm_store_ps(dst + 4, r1);
		_mm_store_ps(dst + 8, r2);
		_mm_store_ps(dst + 12, r3);
		_mm_store_ps(dst + 16, r4);
		_mm_store_ps(dst + 20, r5);
		_mm_store_ps(dst + 24, r6);
		_mm_store_ps(dst + 28, r7);

		dst += 32;
		srcMinX += 8;
		srcMinY += 8;
		srcMaxX += 8;
		srcMaxY += 8;
	}

	uint32_t rem = numRects & 7;
	if (rem >= 4) {
		const __m128 minx0123 = _mm_load_ps(srcMinX + 0);
		const __m128 miny0123 = _mm_load_ps(srcMinY + 0);
		const __m128 maxx0123 = _mm_load_ps(srcMaxX + 0);
		const __m128 maxy0123 = _mm_load_ps(srcMaxY + 0);

		const __m128 minx01_miny01 = _mm_movelh_ps(minx0123, miny0123);
		const __m128 minx23_miny23 = _mm_movehl_ps(miny0123, minx0123);
		const __m128 maxx01_maxy01 = _mm_movelh_ps(maxx0123, maxy0123);
		const __m128 maxx23_maxy23 = _mm_movehl_ps(maxy0123, maxx0123);

		const __m128 r0 = _mm_shuffle_ps(minx01_miny01, maxx01_maxy01, _MM_SHUFFLE(2, 0, 2, 0));
		const __m128 r1 = _mm_shuffle_ps(minx01_miny01, maxx01_maxy01, _MM_SHUFFLE(3, 1, 3, 1));
		const __m128 r2 = _mm_shuffle_ps(minx23_miny23, maxx23_maxy23, _MM_SHUFFLE(2, 0, 2, 0));
		const __m128 r3 = _mm_shuffle_ps(minx23_miny23, maxx23_maxy23, _MM_SHUFFLE(3, 1, 3, 1));

		_mm_store_ps(dst + 0, r0);
		_mm_store_ps(dst + 4, r1);
		_mm_store_ps(dst + 8, r2);
		_mm_store_ps(dst + 12, r3);

		dst += 16;
		srcMinX += 4;
		srcMinY += 4;
		srcMaxX += 4;
		srcMaxY += 4;

		rem -= 4;
	}

	if (rem >= 2) {
		const __m128 minx01 = _mm_loadl_pi(_mm_undefined_ps(), (const __m64*)srcMinX);
		const __m128 miny01 = _mm_loadl_pi(_mm_undefined_ps(), (const __m64*)srcMinY);
		const __m128 maxx01 = _mm_loadl_pi(_mm_undefined_ps(), (const __m64*)srcMaxX);
		const __m128 maxy01 = _mm_loadl_pi(_mm_undefined_ps(), (const __m64*)srcMaxY);

		const __m128 minx01_miny01 = _mm_movelh_ps(minx01, miny01);
		const __m128 maxx01_maxy01 = _mm_movelh_ps(maxx01, maxy01);

		const __m128 r0 = _mm_shuffle_ps(minx01_miny01, maxx01_maxy01, _MM_SHUFFLE(2, 0, 2, 0));
		const __m128 r1 = _mm_shuffle_ps(minx01_miny01, maxx01_maxy01, _MM_SHUFFLE(3, 1, 3, 1));

		_mm_store_ps(dst + 0, r0);
		_mm_store_ps(dst + 4, r1);

		dst += 8;
		srcMinX += 2;
		srcMinY += 2;
		srcMaxX += 2;
		srcMaxY += 2;

		rem -= 2;
	}

	if (rem) {
		dst[0] = *srcMinX;
		dst[1] = *srcMinY;
		dst[2] = *srcMaxX;
		dst[3] = *srcMaxY;
	}
}

void rectIntersect(const RectSoA* soa, uint32_t numRects, const Rect* test, bool* results)
{
	const float* minx = soa->m_MinX;
	const float* miny = soa->m_MinY;
	const float* maxx = soa->m_MaxX;
	const float* maxy = soa->m_MaxY;
	uint32_t* res32 = (uint32_t*)results;

	const __m128 xmm_test = _mm_loadu_ps((const float*)test);
	const __m128 testMinX = _mm_shuffle_ps(xmm_test, xmm_test, _MM_SHUFFLE(0, 0, 0, 0));
	const __m128 testMinY = _mm_shuffle_ps(xmm_test, xmm_test, _MM_SHUFFLE(1, 1, 1, 1));
	const __m128 testMaxX = _mm_shuffle_ps(xmm_test, xmm_test, _MM_SHUFFLE(2, 2, 2, 2));
	const __m128 testMaxY = _mm_shuffle_ps(xmm_test, xmm_test, _MM_SHUFFLE(3, 3, 3, 3));

	const uint32_t iter = numRects >> 3; // 8 rects / iter
	for (uint32_t i = 0; i < iter; ++i) {
		const __m128 culled0_0123 = _mm_or_ps(
			_mm_cmpgt_ps(_mm_load_ps(minx), testMaxX), 
			_mm_cmpgt_ps(_mm_load_ps(miny), testMaxY)
		);
		const __m128 culled1_0123 = _mm_or_ps(
			_mm_cmplt_ps(_mm_load_ps(maxx), testMinX), 
			_mm_cmplt_ps(_mm_load_ps(maxy), testMinY)
		);

		const __m128 culled0_4567 = _mm_or_ps(
			_mm_cmpgt_ps(_mm_load_ps(minx + 4), testMaxX), 
			_mm_cmpgt_ps(_mm_load_ps(miny + 4), testMaxY)
		);
		const __m128 culled1_4567 = _mm_or_ps(
			_mm_cmplt_ps(_mm_load_ps(maxx + 4), testMinX), 
			_mm_cmplt_ps(_mm_load_ps(maxy + 4), testMinY)
		);

		const __m128 culled0123 = _mm_or_ps(culled0_0123, culled1_0123);
		const __m128 culled4567 = _mm_or_ps(culled0_4567, culled1_4567);

		int mask0123 = _mm_movemask_ps(culled0123);
		int mask4567 = _mm_movemask_ps(culled4567);

		res32[0] = s_MaskToVisibility4[mask0123];
		res32[1] = s_MaskToVisibility4[mask4567];
		res32 += 2;

		minx += 8;
		miny += 8;
		maxx += 8;
		maxy += 8;
	}

	uint32_t rem = numRects & 7;
	if (rem >= 4) {
		const __m128 culled0_0123 = _mm_or_ps(
			_mm_cmpgt_ps(_mm_load_ps(minx), testMaxX), 
			_mm_cmpgt_ps(_mm_load_ps(miny), testMaxY)
		);

		const __m128 culled1_0123 = _mm_or_ps(
			_mm_cmplt_ps(_mm_load_ps(maxx), testMinX), 
			_mm_cmplt_ps(_mm_load_ps(maxy), testMinY)
		);

		const __m128 culled0123 = _mm_or_ps(culled0_0123, culled1_0123);

		int mask0123 = _mm_movemask_ps(culled0123);

		res32[0] = s_MaskToVisibility4[mask0123];

		minx += 4;
		miny += 4;
		maxx += 4;
		maxy += 4;
		res32++;

		rem -= 4;
	}

	uint16_t* res16 = (uint16_t*)res32;
	if (rem >= 2) {
		const __m128 culled0_01 = _mm_or_ps(
			_mm_cmpgt_ps(_mm_loadl_pi(_mm_undefined_ps(), (const __m64*)minx), testMaxX),
			_mm_cmpgt_ps(_mm_loadl_pi(_mm_undefined_ps(), (const __m64*)miny), testMaxY)
		);

		const __m128 culled1_01 = _mm_or_ps(
			_mm_cmplt_ps(_mm_loadl_pi(_mm_undefined_ps(), (const __m64*)maxx), testMinX),
			_mm_cmplt_ps(_mm_loadl_pi(_mm_undefined_ps(), (const __m64*)maxy), testMinY)
		);

		const __m128 culled01 = _mm_or_ps(culled0_01, culled1_01);

		int mask01 = _mm_movemask_ps(culled01);

		res16[0] = (uint16_t)s_MaskToVisibility4[mask01];

		minx += 2;
		miny += 2;
		maxx += 2;
		maxy += 2;
		res16++;

		rem -= 2;
	}

	if (rem) {
		if ((minx[0] > test->m_MaxX) ||
			(miny[0] > test->m_MaxY) ||
			(maxx[0] < test->m_MinX) ||
			(maxy[0] < test->m_MinY)) 
		{
			*(uint8_t*)res16 = 0;
		} else {
			*(uint8_t*)res16 = 1;
		}
	}
}

// NOTE: This is the same code as above but the results are packed into a bitset. 
// No measurable performance difference but it uses less memory.
void rectIntersectBitset(const RectSoA* soa, uint32_t numRects, const Rect* test, uint8_t* bitset)
{
	const float* minx = soa->m_MinX;
	const float* miny = soa->m_MinY;
	const float* maxx = soa->m_MaxX;
	const float* maxy = soa->m_MaxY;

	const __m128 xmm_test = _mm_loadu_ps((const float*)test);
	const __m128 testMinX = _mm_shuffle_ps(xmm_test, xmm_test, _MM_SHUFFLE(0, 0, 0, 0));
	const __m128 testMinY = _mm_shuffle_ps(xmm_test, xmm_test, _MM_SHUFFLE(1, 1, 1, 1));
	const __m128 testMaxX = _mm_shuffle_ps(xmm_test, xmm_test, _MM_SHUFFLE(2, 2, 2, 2));
	const __m128 testMaxY = _mm_shuffle_ps(xmm_test, xmm_test, _MM_SHUFFLE(3, 3, 3, 3));

	const uint32_t iter = numRects >> 3; // 8 rects / iter
	for (uint32_t i = 0; i < iter; ++i) {
		const __m128 culled0_0123 = _mm_or_ps(
			_mm_cmpgt_ps(_mm_load_ps(minx), testMaxX),
			_mm_cmpgt_ps(_mm_load_ps(miny), testMaxY)
		);
		const __m128 culled1_0123 = _mm_or_ps(
			_mm_cmplt_ps(_mm_load_ps(maxx), testMinX),
			_mm_cmplt_ps(_mm_load_ps(maxy), testMinY)
		);

		const __m128 culled0_4567 = _mm_or_ps(
			_mm_cmpgt_ps(_mm_load_ps(minx + 4), testMaxX),
			_mm_cmpgt_ps(_mm_load_ps(miny + 4), testMaxY)
		);
		const __m128 culled1_4567 = _mm_or_ps(
			_mm_cmplt_ps(_mm_load_ps(maxx + 4), testMinX),
			_mm_cmplt_ps(_mm_load_ps(maxy + 4), testMinY)
		);

		const __m128 culled0123 = _mm_or_ps(culled0_0123, culled1_0123);
		const __m128 culled4567 = _mm_or_ps(culled0_4567, culled1_4567);

		const uint32_t mask0123 = (uint32_t)_mm_movemask_ps(culled0123);
		const uint32_t mask4567 = (uint32_t)_mm_movemask_ps(culled4567);

		bitset[0] = ~(uint8_t)(mask0123 | (mask4567 << 4));

		bitset++;
		minx += 8;
		miny += 8;
		maxx += 8;
		maxy += 8;
}

	uint32_t rem = numRects & 7;
	if (rem) {
		uint32_t shift = 0;
		bitset[0] = 0;

		if (rem >= 4) {
			const __m128 culled0_0123 = _mm_or_ps(
				_mm_cmpgt_ps(_mm_load_ps(minx), testMaxX),
				_mm_cmpgt_ps(_mm_load_ps(miny), testMaxY)
			);

			const __m128 culled1_0123 = _mm_or_ps(
				_mm_cmplt_ps(_mm_load_ps(maxx), testMinX),
				_mm_cmplt_ps(_mm_load_ps(maxy), testMinY)
			);

			const __m128 culled0123 = _mm_or_ps(culled0_0123, culled1_0123);

			const uint32_t mask0123 = (uint32_t)_mm_movemask_ps(culled0123);

			bitset[0] |= (uint8_t)(mask0123 & 0x0F);
			shift = 4;

			minx += 4;
			miny += 4;
			maxx += 4;
			maxy += 4;

			rem -= 4;
		}

		if (rem >= 2) {
			const __m128 culled0_01 = _mm_or_ps(
				_mm_cmpgt_ps(_mm_loadl_pi(_mm_undefined_ps(), (const __m64*)minx), testMaxX),
				_mm_cmpgt_ps(_mm_loadl_pi(_mm_undefined_ps(), (const __m64*)miny), testMaxY)
			);

			const __m128 culled1_01 = _mm_or_ps(
				_mm_cmplt_ps(_mm_loadl_pi(_mm_undefined_ps(), (const __m64*)maxx), testMinX),
				_mm_cmplt_ps(_mm_loadl_pi(_mm_undefined_ps(), (const __m64*)maxy), testMinY)
			);

			const __m128 culled01 = _mm_or_ps(culled0_01, culled1_01);

			const uint32_t mask01 = (uint32_t)_mm_movemask_ps(culled01);

			bitset[0] |= (uint8_t)((mask01 & 0x03) << shift);
			shift += 2;

			minx += 2;
			miny += 2;
			maxx += 2;
			maxy += 2;

			rem -= 2;
		}

		if (rem) {
			if ((minx[0] > test->m_MaxX) ||
				(miny[0] > test->m_MaxY) ||
				(maxx[0] < test->m_MinX) ||
				(maxy[0] < test->m_MinY)) {
				bitset[0] |= (uint8_t)(1 << shift);
			}
		}

		bitset[0] = ~bitset[0];
	}
}

#else // !JX_CONFIG_MATH_SIMD
void rectAoSToSoA(const Rect* aos, uint32_t numRects, RectSoA* soa)
{
	for (uint32_t i = 0; i < numRects; ++i) {
		const Rect* r = &aos[i];

		soa->m_MinX[i] = r->m_MinX;
		soa->m_MinY[i] = r->m_MinY;
		soa->m_MaxX[i] = r->m_MaxX;
		soa->m_MaxY[i] = r->m_MaxY;
	}
}

void rectSoAToAoS(const RectSoA* soa, uint32_t numRects, Rect* aos)
{
	for (uint32_t i = 0; i < numRects; ++i) {
		Rect* r = &aos[i];

		r->m_MinX = soa->m_MinX[i];
		r->m_MinY = soa->m_MinY[i];
		r->m_MaxX = soa->m_MaxX[i];
		r->m_MaxY = soa->m_MaxY[i];
	}
}

void rectIntersect(const RectSoA* soa, uint32_t numRects, const Rect* test, bool* results)
{
#error "Not implemented yet"
}
void rectIntersectBitset(const RectSoA* soa, uint32_t numRects, const Rect* test, uint8_t* bitset)
{
#error "Not implemented yet"
}
#endif
}
