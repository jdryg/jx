#ifndef JX_BITSET_H
#define JX_BITSET_H

#include <stdint.h>

namespace bx
{
struct AllocatorI;
}

namespace jx
{
struct BitSet
{
	uint64_t* m_Bits;
	uint32_t m_Size;
};

struct BitSetIter
{
	uint32_t m_QWordID;
	uint64_t m_Bits;
};

BitSet* createBitSet(bx::AllocatorI* allocator, uint32_t numBits);
void destroyBitSet(bx::AllocatorI* allocator, BitSet* bs);

void resizeBitSet(bx::AllocatorI* allocator, BitSet* bs, uint32_t numBits);

void makeBitSet(BitSet* bs, uint64_t* mem, uint32_t numBits);

uint32_t bitSetCalcMemorySize(uint32_t numBits);
uint32_t bitSetGetNumBits(const BitSet* bs);

void bitSetClear(BitSet* bs);
bool bitSetGetBit(const BitSet* bs, uint32_t bit);
void bitSetSetBit(BitSet* bs, uint32_t bit, bool val);
void bitSetToggleBit(BitSet* bs, uint32_t bit);

bool bitSetIsCleared(const BitSet* bs);

void bitSetIterBegin(const BitSet* bs, BitSetIter* iter, uint32_t firstBit);
uint32_t bitSetIterNext(const BitSet* bs, BitSetIter* iter);
}

#include "inline/bitset.inl"

#endif
