#ifndef JX_BITSET_H
#error "Must be included from jx/bitset.h"
#endif

#include <bx/uint32_t.h>
#include <jx/sys.h>

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

inline void bitSetIterBegin(const BitSet* bs, BitSetIter* iter, uint32_t firstBit)
{
	JX_CHECK((firstBit >> 6) <= bs->m_Size, "Invalid first bit");
	const uint32_t qwordID = firstBit >> 6;
	const uint32_t bitID = firstBit & 63;
	iter->m_QWordID = qwordID;

	const uint64_t bitset = (bs->m_Size != 0) ? bs->m_Bits[qwordID] : 0;
	iter->m_Bits = bitset & (~((1ull << bitID) - 1));
}

BX_PRAGMA_DIAGNOSTIC_PUSH()
BX_PRAGMA_DIAGNOSTIC_IGNORED_MSVC(4146) // unary minus operator applied to unsigned type, result still unsigned
inline uint32_t bitSetIterNext(const BitSet* bs, BitSetIter* iter)
{
	while (iter->m_Bits == 0 && iter->m_QWordID < bs->m_Size) {
		iter->m_QWordID++;
		iter->m_Bits = bs->m_Bits[iter->m_QWordID];
	}
	
	const uint64_t bitset = iter->m_Bits;
	if (bitset) {
		const uint64_t t = bitset & -bitset;
		const uint32_t r = bx::uint64_cnttz(bitset);
		iter->m_Bits ^= t;
		return iter->m_QWordID * 64 + r;
	}

	return UINT32_MAX;
}
BX_PRAGMA_DIAGNOSTIC_POP()
}
