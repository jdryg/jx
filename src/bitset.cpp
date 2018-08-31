#include <jx/bitset.h>
#include <jx/sys.h>
#include <bx/allocator.h>

namespace jx
{
BitSet* createBitSet(bx::AllocatorI* allocator, uint32_t numBits)
{
	BitSet* bs = (BitSet*)BX_ALLOC(allocator, sizeof(BitSet));

	const uint32_t memSize = bitSetCalcMemorySize(numBits);
	bs->m_Bits = (uint64_t*)BX_ALIGNED_ALLOC(allocator, memSize, 16);
	bs->m_Size = memSize / sizeof(uint64_t);

	return bs;
}

void destroyBitSet(bx::AllocatorI* allocator, BitSet* bs)
{
	BX_ALIGNED_FREE(allocator, bs->m_Bits, 16);
	BX_FREE(allocator, bs);
}

void resizeBitSet(bx::AllocatorI* allocator, BitSet* bs, uint32_t numBits)
{
	const uint32_t oldMemSize = sizeof(uint64_t) * bs->m_Size;
	const uint32_t memSize = bitSetCalcMemorySize(numBits);
	if (memSize <= oldMemSize) {
		return;
	}

	bs->m_Bits = (uint64_t*)BX_ALIGNED_REALLOC(allocator, bs->m_Bits, memSize, 16);
	bx::memSet((uint8_t*)bs->m_Bits + oldMemSize, 0, memSize - oldMemSize);
	bs->m_Size = memSize / sizeof(uint64_t);
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

bool bitSetIsCleared(const BitSet* bs)
{
	const uint32_t sz = bs->m_Size;
	for (uint32_t i = 0; i < sz; ++i) {
		if (bs->m_Bits[i] != 0) {
			return false;
		}
	}

	return true;
}
}
