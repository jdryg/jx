#ifndef JX_MEMORY_TRACER_H
#define JX_MEMORY_TRACER_H

#include <stdint.h>
#include <stddef.h> // size_t

namespace bx
{
struct AllocatorI;
}

namespace jx
{
bool memTracerInit(bx::AllocatorI* allocator);
void memTracerShutdown();

uint16_t memTracerCreateAllocator(const char* name);
void memTracerDestroyAllocator(uint16_t allocatorID);
void memTracerOnRealloc(uint16_t allocatorID, void* ptr, void* newPtr, size_t newSize, const char* file, uint32_t line);
}

#endif
