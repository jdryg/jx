#ifndef JX_TRACING_ALLOCATOR_H
#define JX_TRACING_ALLOCATOR_H

#include <bx/allocator.h>

namespace jx
{
class TracingAllocator : public bx::AllocatorI
{
public:
	TracingAllocator(const char* name, bx::AllocatorI* parentAllocator);
	virtual ~TracingAllocator();

	virtual void* realloc(void* _ptr, size_t _size, size_t _align, const char* _file, uint32_t _line);

private:
	bx::AllocatorI* m_ParentAllocator;
	uint16_t m_TracerID;
};
}

#endif
