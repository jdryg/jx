#ifndef JX_OBJECT_POOL_H
#define JX_OBJECT_POOL_H

#include <stdint.h>

namespace bx
{
struct AllocatorI;
}

namespace jx
{
struct ObjectPool;

ObjectPool* createObjectPool(uint32_t objSize, uint32_t numObjsPerBlock, bx::AllocatorI* allocator);
void destroyObjectPool(ObjectPool* pool);

void* objPoolAlloc(ObjectPool* pool);
void objPoolFree(ObjectPool* pool, void* obj);
}

#endif
