#include <jx/stack_allocator.h>
#include <jx/sys.h>

BX_PRAGMA_DIAGNOSTIC_IGNORED_CLANG_GCC("-Wunused-private-field")

namespace jx
{
struct AllocHeader
{
	uint32_t m_ID;
	uint32_t m_Offset;
};

inline uint32_t alignSize(uint32_t sz, uint32_t alignment)
{
	JX_CHECK(bx::isPowerOf2<uint32_t>(alignment), "Invalid alignment value");
	const uint32_t mask = alignment - 1;
	return (sz & (~mask)) + ((sz & mask) != 0 ? alignment : 0);
}

StackAllocator::StackAllocator(void* buffer, uint32_t size)
	: m_Buffer((uint8_t*)buffer)
	, m_Ptr((uint8_t*)buffer)
	, m_Size(size)
	, m_LastAllocationID(0)
{
}

StackAllocator::~StackAllocator()
{
}

void* StackAllocator::realloc(void* _ptr, size_t _size, size_t _align, const char* _file, uint32_t _line)
{
	BX_UNUSED(_file, _line);
	JX_CHECK(!_ptr || (_ptr && !_size), "StackAllocator doesn't support reallocations");
	JX_CHECK(!_ptr || (_ptr && _ptr >= m_Buffer && _ptr < m_Buffer + m_Size), "Invalid pointer passed to StackAllocator");

	_align = bx::max<size_t>(_align, (size_t)8);

	if (_ptr) {
		if (_size) {
			// Reallocation (unsupported)
			// TODO: Support reallocations on the last allocation only?
			return nullptr;
		}

		// Free
		const AllocHeader* hdr = (const AllocHeader*)((const uint8_t*)_ptr - sizeof(AllocHeader));
		JX_CHECK(hdr->m_ID == m_LastAllocationID, "Deallocations should happen in LIFO order");
		
		if (hdr->m_ID == m_LastAllocationID) {
			uint8_t* allocStart = (uint8_t*)_ptr - hdr->m_Offset;
			m_Ptr = allocStart;
			m_LastAllocationID--;
		}

		return nullptr;
	}

	// Allocate
	uint8_t* ptr = (uint8_t*)bx::alignPtr(m_Ptr, sizeof(AllocHeader), _align);
	AllocHeader* hdr = (AllocHeader*)(ptr - sizeof(AllocHeader));
	hdr->m_ID = ++m_LastAllocationID;
	hdr->m_Offset = (uint32_t)(ptr - m_Ptr);

	m_Ptr = ptr + _size;

	return ptr;
}
}
