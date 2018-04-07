#include <jx/object_pool.h>
#include <jx/sys.h>
#include <bx/allocator.h>

namespace jx
{
struct PoolChunk
{
	PoolChunk* m_Next;
	uint8_t* m_MemoryBlock;
	uint8_t* m_NextFreeElement;
	uint32_t m_NumFreeElements;
	uint32_t m_NumInitialized;
};

struct ObjectPool
{
	PoolChunk* m_ChunkList;
	bx::AllocatorI* m_Allocator;
	uint32_t m_ObjSize;
	uint32_t m_NumObjsPerChunk;
};

static PoolChunk* allocNewPoolChunk(ObjectPool* pool);
static void* allocFromChunk(const ObjectPool* pool, PoolChunk* chunk);
static void freeFromChunk(const ObjectPool* pool, PoolChunk* chunk, void* ptr);

ObjectPool* createObjectPool(uint32_t objSize, uint32_t numObjsPerBlock, bx::AllocatorI* allocator)
{
	JX_CHECK(objSize != 0, "Invalid object size passed to object pool");
	JX_CHECK(allocator != nullptr, "Invalid allocator passed to object pool");

	ObjectPool* pool = (ObjectPool*)BX_ALLOC(allocator, sizeof(ObjectPool));
	if (!pool) {
		return nullptr;
	}

	bx::memSet(pool, 0, sizeof(ObjectPool));
	pool->m_Allocator = allocator;
	pool->m_ObjSize = objSize;
	pool->m_NumObjsPerChunk = numObjsPerBlock;

	return pool;
}

void destroyObjectPool(ObjectPool* pool)
{
	bx::AllocatorI* allocator = pool->m_Allocator;

	PoolChunk* chunk = pool->m_ChunkList;
	while (chunk) {
		PoolChunk* nextChunk = chunk->m_Next;
		BX_FREE(allocator, chunk);
		chunk = nextChunk;
	}

	BX_FREE(allocator, pool);
}

void* objPoolAlloc(ObjectPool* pool)
{
	PoolChunk* chunk = pool->m_ChunkList;
	while (chunk) {
		void* mem = allocFromChunk(pool, chunk);
		if (mem) {
			return mem;
		}

		// Move on to the next chunk...
		chunk = chunk->m_Next;
	}

	// No free memory. Allocate a new chunk and get an element from it.
	chunk = allocNewPoolChunk(pool);
	if (!chunk) {
		JX_CHECK(false, "Failed to allocate memory for object pool chunk");
		return nullptr;
	}

	return allocFromChunk(pool, chunk);
}

void objPoolFree(ObjectPool* pool, void* obj)
{
	const uint64_t chunkSize = pool->m_ObjSize * pool->m_NumObjsPerChunk;

	// Find the parent chunk...
	PoolChunk* chunk = pool->m_ChunkList;
	while (chunk) {
		if (obj >= chunk->m_MemoryBlock && obj < (chunk->m_MemoryBlock + chunkSize)) {
			freeFromChunk(pool, chunk, obj);
			return;
		}

		// Move on to the next chunk...
		chunk = chunk->m_Next;
	}

	JX_CHECK(false, "Tried to free object from another pool");
}

//////////////////////////////////////////////////////////////////////////
// Internal
//
inline uint8_t* addrFromIndex(const ObjectPool* pool, PoolChunk* chunk, uint32_t i)
{
	return chunk->m_MemoryBlock + (i * pool->m_ObjSize);
}

inline uint32_t indexFromAddr(const ObjectPool* pool, PoolChunk* chunk, const uint8_t* p)
{
	return (uint32_t)((((uint32_t)(p - chunk->m_MemoryBlock)) / pool->m_ObjSize));
}

static void* allocFromChunk(const ObjectPool* pool, PoolChunk* chunk)
{
	const uint32_t numObjsPerChunk = pool->m_NumObjsPerChunk;

	if (chunk->m_NumInitialized < numObjsPerChunk) {
		uint32_t* p = (uint32_t*)addrFromIndex(pool, chunk, chunk->m_NumInitialized);
		*p = chunk->m_NumInitialized + 1;
		chunk->m_NumInitialized++;
	}

	void* ret = nullptr;
	if (chunk->m_NumFreeElements > 0) {
		ret = (void*)chunk->m_NextFreeElement;
		--chunk->m_NumFreeElements;
		if (chunk->m_NumFreeElements != 0) {
			chunk->m_NextFreeElement = addrFromIndex(pool, chunk, *((uint32_t*)chunk->m_NextFreeElement));
		} else {
			chunk->m_NextFreeElement = nullptr;
		}
	}

	return ret;
}

static void freeFromChunk(const ObjectPool* pool, PoolChunk* chunk, void* ptr)
{
	if (chunk->m_NextFreeElement != nullptr) {
		(*(uint32_t*)ptr) = indexFromAddr(pool, chunk, chunk->m_NextFreeElement);
		chunk->m_NextFreeElement = (uint8_t*)ptr;
	} else {
		(*(uint32_t*)ptr) = pool->m_NumObjsPerChunk;
		chunk->m_NextFreeElement = (uint8_t*)ptr;
	}

	++chunk->m_NumFreeElements;
	JX_CHECK(chunk->m_NumFreeElements <= pool->m_NumObjsPerChunk, "Freed too many elements");
}

static PoolChunk* allocNewPoolChunk(ObjectPool* pool)
{
	const uint64_t chunkSize = pool->m_ObjSize * pool->m_NumObjsPerChunk + sizeof(PoolChunk);

	uint8_t* mem = (uint8_t*)BX_ALLOC(pool->m_Allocator, chunkSize);

	PoolChunk* chunk = (PoolChunk*)mem;
	chunk->m_MemoryBlock = mem + sizeof(PoolChunk);
	chunk->m_NumFreeElements = pool->m_NumObjsPerChunk;
	chunk->m_NextFreeElement = chunk->m_MemoryBlock;
	chunk->m_NumInitialized = 0;
	chunk->m_Next = pool->m_ChunkList;
	pool->m_ChunkList = chunk;

	return chunk;
}
}
