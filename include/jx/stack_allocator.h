#ifndef JX_STACK_ALLOCATOR_H
#define JX_STACK_ALLOCATOR_H

#include <stdint.h>
#include <bx/allocator.h>

namespace jx
{
class StackAllocator : public bx::AllocatorI
{
public:
	StackAllocator(void* buffer, uint32_t size);
	virtual ~StackAllocator();

	virtual void* realloc(void* _ptr, size_t _size, size_t _align, const char* _file, uint32_t _line);

private:
	uint8_t* m_Buffer;
	uint8_t* m_Ptr;
	uint32_t m_Size;
	uint32_t m_LastAllocationID;
};
}

#endif
