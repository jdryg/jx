#ifndef JX_RECT_H
#define JX_RECT_H

#include <stdint.h>
#include <bx/math.h>

namespace jx
{
struct Vec2f;

struct Rect
{
	float m_MinX, m_MinY;
	float m_MaxX, m_MaxY;
};

struct RectSoA
{
	float* m_MinX;
	float* m_MinY;
	float* m_MaxX;
	float* m_MaxY;
};

void rectAoSToSoA(const Rect* aos, uint32_t numRects, RectSoA* soa);
void rectSoAToAoS(const RectSoA* soa, uint32_t numRects, Rect* aos);

void rectIntersect(const RectSoA* soa, uint32_t numRects, const Rect* test, bool* results);
void rectIntersectBitset(const RectSoA* soa, uint32_t numRects, const Rect* test, uint8_t* bitset);

void rectCalcFromPointList(const float* points, uint32_t numPoints, Rect* rect);

bool rectLineSegmentIntersection(const Rect* rect, const jx::Vec2f& s, const jx::Vec2f& e);

inline void rectInflate(Rect* rect, float x, float y)
{
	const float half_x = x * 0.5f;
	const float half_y = y * 0.5f;
	rect->m_MinX -= half_x;
	rect->m_MinY -= half_y;
	rect->m_MaxX += half_x;
	rect->m_MaxY += half_y;
}

inline void rectExpandToInclude(Rect* rect, float x, float y)
{
	rect->m_MinX = bx::min<float>(rect->m_MinX, x);
	rect->m_MinY = bx::min<float>(rect->m_MinY, y);
	rect->m_MaxX = bx::max<float>(rect->m_MaxX, x);
	rect->m_MaxY = bx::max<float>(rect->m_MaxY, y);
}
}

#endif
