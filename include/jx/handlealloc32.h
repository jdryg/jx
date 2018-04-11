#ifndef JX_HANDLEALLOC32_H
#define JX_HANDLEALLOC32_H

#include <stdint.h>

namespace bx
{
struct AllocatorI;
}

namespace jx
{
struct HandleAlloc32;

HandleAlloc32* createHandleAlloc32(bx::AllocatorI* allocator);
void destroyHandleAlloc32(HandleAlloc32* ha);

uint32_t ha32AllocHandles(HandleAlloc32* ha, uint32_t n);
void ha32FreeHandles(HandleAlloc32* ha, uint32_t firstHandle, uint32_t n);
bool ha32AllocSpecificHandle(HandleAlloc32* ha, uint32_t handle);
uint32_t ha32GetCapacity(HandleAlloc32* ha);
}

#endif
