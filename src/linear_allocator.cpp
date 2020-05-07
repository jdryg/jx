#include <jx/linear_allocator.h>
#include <jx/sys.h>

namespace jx
{
static const uint32_t kLinearAllocatorChunkAlignment = 16;

inline uint32_t alignSize(uint32_t sz, uint32_t alignment)
{
	JX_CHECK(bx::isPowerOf2<uint32_t>(alignment), "Invalid alignment value");
	const uint32_t mask = alignment - 1;
	return (sz & (~mask)) + ((sz & mask) != 0 ? alignment : 0);
}

LinearAllocator::LinearAllocator(bx::AllocatorI* parentAllocator, uint32_t minChunkSize)
	: m_ParentAllocator(parentAllocator)
	, m_ChunkListHead(nullptr)
	, m_ChunkListTail(nullptr)
	, m_CurChunk(nullptr)
	, m_MinChunkSize(minChunkSize)
{
	JX_CHECK(bx::isPowerOf2(m_MinChunkSize), "Linear allocator chunk size should be a power of 2");
}

LinearAllocator::~LinearAllocator()
{
	Chunk* c = m_ChunkListHead;
	while (c) {
		Chunk* next = c->m_Next;
		BX_ALIGNED_FREE(m_ParentAllocator, c, kLinearAllocatorChunkAlignment);
		c = next;
	}
	m_ChunkListHead = nullptr;
	m_ChunkListTail = nullptr;
	m_CurChunk = nullptr;
}

void* LinearAllocator::realloc(void* _ptr, size_t _size, size_t _align, const char* _file, uint32_t _line)
{
	BX_UNUSED(_file, _line);
	JX_CHECK(!_ptr || (_ptr && !_size), "LinearAllocator doesn't support reallocations");
	JX_CHECK(!_ptr || (_ptr && _ptr >= m_Buffer && _ptr < m_Buffer + m_Size), "Invalid pointer passed to LinearAllocator");

	_align = bx::max<size_t>(_align, (size_t)8);

	if (_ptr) {
		// // Reallocation (unsupported) or Free (ignore)
		return nullptr;
	}

	while (m_CurChunk) {
		void* ptr = allocFromChunk(m_CurChunk, _size, _align);
		if (ptr != nullptr) {
			return ptr;
		}

		m_CurChunk = m_CurChunk->m_Next;
	}

	// Allocate new chunk.
	const uint32_t chunkCapacity = alignSize((uint32_t)_size, m_MinChunkSize);
	const uint32_t totalMemory = 0
		+ alignSize(sizeof(Chunk), kLinearAllocatorChunkAlignment)
		+ chunkCapacity
		;

	uint8_t* mem = (uint8_t*)BX_ALIGNED_ALLOC(m_ParentAllocator, totalMemory, kLinearAllocatorChunkAlignment);
	if (mem == nullptr) {
		// Failed to allocate new chunk.
		return nullptr;
	}

	Chunk* c = (Chunk*)mem; mem += alignSize(sizeof(Chunk), kLinearAllocatorChunkAlignment);
	c->m_Buffer = mem;      mem += chunkCapacity;

	c->m_Capacity = chunkCapacity;
	c->m_Offset = 0;
	c->m_Next = nullptr;

	if (m_ChunkListHead == nullptr) {
		JX_CHECK(m_ChunkListTail == nulltptr, "Invalid LinearAllocator state");
		m_ChunkListHead = c;
		m_ChunkListTail = c;
	} else if (m_ChunkListTail != nullptr) {
		m_ChunkListTail->m_Next = c;
	}
	m_CurChunk = c;

	return allocFromChunk(m_CurChunk, _size, _align);
}

void LinearAllocator::freeAll()
{
	Chunk* c = m_ChunkListHead;
	while (c != nullptr) {
		c->m_Offset = 0;
		c = c->m_Next;
	}
	m_CurChunk = m_ChunkListHead;
}

void* LinearAllocator::allocFromChunk(Chunk* c, size_t size, size_t align)
{
	uintptr_t offset = (uintptr_t)bx::alignPtr(c->m_Buffer + c->m_Offset, 0, align) - (uintptr_t)c->m_Buffer;
	if (offset + size > c->m_Capacity) {
		return nullptr;
	}

	void* ptr = &c->m_Buffer[offset];
	c->m_Offset = (uint32_t)(offset + size);

	return ptr;
}
}
