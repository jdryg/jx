#ifndef JX_RECT_H
#define JX_RECT_H

#include <stdint.h>

namespace jx
{
struct BitSet;

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
void rectIntersect(const RectSoA* soa, uint32_t numRects, const Rect* test, BitSet* results);
}

#endif
