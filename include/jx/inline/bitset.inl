#ifndef JX_BITSET_H
#error "Must be included from jx/bitset.h"
#endif

namespace jx
{
inline uint32_t bitSetCalcMemorySize(uint32_t numBits)
{
	const uint32_t numQWords = (numBits >> 6) + ((numBits & 63) != 0 ? 1 : 0);
	return numQWords * sizeof(uint64_t);
}

inline uint32_t bitSetGetNumBits(const BitSet* bs)
{
	return (bs->m_Size << 6);
}

inline bool bitSetGetBit(const BitSet* bs, uint32_t bit)
{
	const uint32_t qwordID = bit >> 6;
	const uint32_t bitID = bit & 63;
	return (bs->m_Bits[qwordID] & (1ull << bitID)) != 0;
}

inline void bitSetSetBit(BitSet* bs, uint32_t bit, bool val)
{
	const uint32_t qwordID = bit >> 6;
	const uint32_t bitID = bit & 63;
	if (val) {
		bs->m_Bits[qwordID] |= (1ull << bitID);
	} else {
		bs->m_Bits[qwordID] &= ~(1ull << bitID);
	}
}

inline void bitSetToggleBit(BitSet* bs, uint32_t bit)
{
	const uint32_t qwordID = bit >> 6;
	const uint32_t bitID = bit & 63;
	bs->m_Bits[qwordID] ^= (1ull << bitID);
}
}