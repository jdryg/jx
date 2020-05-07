#ifndef JX_LINEAR_ALLOCATOR_H
#define JX_LINEAR_ALLOCATOR_H

#include <stdint.h>
#include <bx/allocator.h>

namespace jx
{
class LinearAllocator : public bx::AllocatorI
{
public:
	LinearAllocator(bx::AllocatorI* parentAllocator, uint32_t minChunkSize);
	virtual ~LinearAllocator();

	virtual void* realloc(void* _ptr, size_t _size, size_t _align, const char* _file, uint32_t _line);

	void freeAll();

private:
	struct Chunk
	{
		Chunk* m_Next;
		uint8_t* m_Buffer;
		uint32_t m_Offset;
		uint32_t m_Capacity;
	};

	bx::AllocatorI* m_ParentAllocator;
	Chunk* m_ChunkListHead;
	Chunk* m_ChunkListTail;
	Chunk* m_CurChunk;
	uint32_t m_MinChunkSize;

	void* allocFromChunk(Chunk* c, size_t size, size_t align);
};
}

#endif
