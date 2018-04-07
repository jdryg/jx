#include "tracing_allocator.h"
#include "memory_tracer.h"
#include <bx/string.h>

namespace jx
{
TracingAllocator::TracingAllocator(const char* name, bx::AllocatorI* parentAllocator) 
	: m_ParentAllocator(parentAllocator)
	, m_TracerID(UINT16_MAX)
{
	m_TracerID = memTracerCreateAllocator(name);
}

TracingAllocator::~TracingAllocator()
{
	memTracerDestroyAllocator(m_TracerID);
}

void* TracingAllocator::realloc(void* _ptr, size_t _size, size_t _align, const char* _file, uint32_t _line)
{
	void* newPtr = m_ParentAllocator->realloc(_ptr, _size, _align, _file, _line);

	memTracerOnRealloc(m_TracerID, _ptr, newPtr, _size, _file, _line);

	return newPtr;
}
}