#include <jx/bitset.h>
#include <jx/sys.h>
#include <bx/allocator.h>

namespace jx
{
BitSet* createBitSet(bx::AllocatorI* allocator, uint32_t numBits)
{
	const uint32_t memSize = bitSetCalcMemorySize(numBits);
	uint8_t* mem = (uint8_t*)BX_ALLOC(allocator, sizeof(BitSet) + memSize);

	BitSet* bs = (BitSet*)mem;
	bs->m_Bits = (uint64_t*)(mem + sizeof(BitSet));
	bs->m_Size = memSize / sizeof(uint64_t);

	return bs;
}

void destroyBitSet(bx::AllocatorI* allocator, BitSet* bs)
{
	BX_FREE(allocator, bs);
}

void makeBitSet(BitSet* bs, uint64_t* mem, uint32_t numBits)
{
	JX_CHECK((numBits & 63) == 0, "BitSet size should be a multiple of 64");
	bs->m_Bits = mem;
	bs->m_Size = numBits >> 6;
}

void bitSetClear(BitSet* bs)
{
	bx::memSet(bs->m_Bits, 0, sizeof(uint64_t) * bs->m_Size);
}
}
