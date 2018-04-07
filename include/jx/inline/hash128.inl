#ifndef JX_HASH128_H
#error "Should be included from hash128.h"
#endif

namespace jx
{
inline hash128::hash128(uint64_t seed1, uint64_t seed2)
{
	m_Value[0] = seed1;
	m_Value[1] = seed2;
}

inline hash128::~hash128()
{
}

inline bool hash128::operator < (const hash128& other) const
{
	if (m_Value[1] < other.m_Value[1]) {
		return true;
	} else if (m_Value[1] == other.m_Value[1]) {
		return m_Value[0] < other.m_Value[0];
	}

	return false;
}

inline bool hash128::operator == (const hash128& other) const
{
	return m_Value[0] == other.m_Value[0] && m_Value[1] == other.m_Value[1];
}
}
