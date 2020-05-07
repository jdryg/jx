#ifndef JX_ALLOCATOR_H
#define JX_ALLOCATOR_H

#include <jx/sys.h> // jx::getGlobalAllocator()
#include <type_traits> // std::enable_if
#include <bx/allocator.h> // BX_xxx macros

namespace jx
{
template <typename ObjectT, typename std::enable_if<std::is_class<ObjectT>::value, bool>::type = true>
inline ObjectT* allocArray(bx::AllocatorI* allocator, size_t numElements, size_t align = 0, const char* file = nullptr, unsigned int line = 0)
{
	JX_CHECK(align == 0, "Does not work with align != 0");

	size_t totalSize = sizeof(ObjectT) * numElements + sizeof(size_t);
	char* mem = (char*)allocator->realloc(nullptr, totalSize, align, file, line);

	ObjectT* ptr = (ObjectT*)(mem + sizeof(size_t));
	for (size_t i = 0; i < numElements; ++i) {
		BX_PLACEMENT_NEW(ptr + i, ObjectT)();
	}

	size_t* n = (size_t*)mem;
	*n = numElements;

	return reinterpret_cast<ObjectT*>(mem + sizeof(size_t));
}

template <typename ObjectT, typename std::enable_if<!std::is_class<ObjectT>::value, bool>::type = false>
inline ObjectT* allocArray(bx::AllocatorI* allocator, size_t numElements, size_t align = 0, const char* file = nullptr, unsigned int line = 0)
{
	size_t totalSize = sizeof(ObjectT) * numElements;
	char* mem = (char*)allocator->realloc(nullptr, totalSize, align, file, line);
	return reinterpret_cast<ObjectT*>(mem);
}

template <typename ObjectT, typename std::enable_if<std::is_class<ObjectT>::value, bool>::type = true>
inline void deleteArray(bx::AllocatorI* allocator, ObjectT* ptr, size_t align = 0, const char* file = nullptr, unsigned int line = 0)
{
	char* mem = (char*)ptr;
	size_t* n = (size_t*)(mem - sizeof(size_t));

	for (size_t i = 0; i < *n; ++i) {
		ptr[i].~ObjectT();
	}

	allocator->realloc(n, 0, align, file, line);
}

template <typename ObjectT, typename std::enable_if<!std::is_class<ObjectT>::value, bool>::type = false>
inline void deleteArray(bx::AllocatorI* allocator, ObjectT* ptr, size_t align = 0, const char* file = nullptr, unsigned int line = 0)
{
	allocator->realloc(ptr, 0, align, file, line);
}

#define JX_ALLOC(_size)                              BX_ALLOC(jx::getGlobalAllocator(), _size)
#define JX_REALLOC(_ptr, _size)                      BX_REALLOC(jx::getGlobalAllocator(), _ptr, _size)
#define JX_FREE(_ptr)                                BX_FREE(jx::getGlobalAllocator(), _ptr)
#define JX_ALIGNED_ALLOC(_size, _align)              BX_ALIGNED_ALLOC(jx::getGlobalAllocator(), _size, _align)
#define JX_ALIGNED_REALLOC(_ptr, _size, _align)      BX_ALIGNED_REALLOC(jx::getGlobalAllocator(), _ptr, _size, _align)
#define JX_ALIGNED_FREE(_ptr, _align)                BX_ALIGNED_FREE(jx::getGlobalAllocator(), _ptr, _align)
#define JX_NEW(_type)                                BX_PLACEMENT_NEW(JX_ALLOC(sizeof(_type)), _type)
#define JX_DELETE(_ptr)                              bx::deleteObject(jx::getGlobalAllocator(), _ptr, 0, __FILE__, __LINE__)
#define JX_ALIGNED_NEW(_type, _align)                BX_PLACEMENT_NEW(JX_ALIGNED_ALLOC(sizeof(_type), _align), _type)
#define JX_ALIGNED_DELETE(_ptr, _align)              bx::deleteObject(jx::getGlobalAllocator(), _ptr, _align, __FILE__, __LINE__)
#define JX_NEW_ARRAY(_type, size)                    jx::allocArray<_type>(jx::getGlobalAllocator(), size, 0, __FILE__, __LINE__)
#define JX_DELETE_ARRAY(_ptr)                        jx::deleteArray(jx::getGlobalAllocator(), _ptr, 0, __FILE__, __LINE__)

#define JX_FRAME_ALLOC(_size)                         BX_ALLOC(jx::getFrameAllocator(), _size)
#define JX_FRAME_ALIGNED_ALLOC(_size, _align)         BX_ALIGNED_ALLOC(jx::getFrameAllocator(), _size, _align)
#define JX_FRAME_NEW(_type)                           BX_PLACEMENT_NEW(JX_FRAME_ALLOC(sizeof(_type)), _type)
#define JX_FRAME_ALIGNED_NEW(_type, _align)           BX_PLACEMENT_NEW(JX_FRAME_ALIGNED_ALLOC(sizeof(_type), _align), _type)
#define JX_FRAME_NEW_ARRAY(_type, size)               jx::allocArray<_type>(jx::getFrameAllocator(), size, 0, __FILE__, __LINE__)
}

#endif
