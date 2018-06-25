#include <jx/memory_stream.h>
#include <jx/spooky_hash.h>
#include <bx/allocator.h>
#include <bx/uint32_t.h>

namespace jx
{
#define MIN_BLOCK_SIZE 4096u

struct MemoryStream
{
	bx::AllocatorI* m_Allocator;
	uint8_t* m_Data;
	uint8_t* m_Ptr;
	uint32_t m_Size;
	uint32_t m_Capacity;
};

MemoryStream* createMemoryStream(bx::AllocatorI* allocator)
{
	MemoryStream* ms = (MemoryStream*)BX_ALLOC(allocator, sizeof(MemoryStream));
	if (!ms) {
		return nullptr;
	}

	bx::memSet(ms, 0, sizeof(MemoryStream));

	ms->m_Allocator = allocator;

	return ms;
}

void destroyMemoryStream(MemoryStream* ms)
{
	bx::AllocatorI* allocator = ms->m_Allocator;

	BX_FREE(allocator, ms->m_Data);
	BX_FREE(allocator, ms);
}

bool memStreamCopy(MemoryStream* dst, const MemoryStream* src)
{
	bx::AllocatorI* allocator = dst->m_Allocator;

	const uint32_t sz = src->m_Size;

	dst->m_Data = (uint8_t*)BX_REALLOC(allocator, dst->m_Data, sz);
	if (!dst->m_Data) {
		return false;
	}

	dst->m_Capacity = sz;
	dst->m_Size = sz;
	
	bx::memCopy(dst->m_Data, src->m_Data, sz);
	dst->m_Ptr = dst->m_Data + sz;

	return true;
}

bool memStreamWrite(MemoryStream* ms, const void* data, uint32_t size)
{
	if (ms->m_Size + size > ms->m_Capacity) {
		const uint32_t newCapacity = ms->m_Capacity + bx::uint32_max(MIN_BLOCK_SIZE, size);

		const ptrdiff_t curPos = ms->m_Ptr - ms->m_Data;
		ms->m_Data = (uint8_t*)BX_REALLOC(ms->m_Allocator, ms->m_Data, newCapacity);
		if (!ms->m_Data) {
			return false;
		}

		ms->m_Ptr = ms->m_Data + curPos;
		ms->m_Capacity = newCapacity;
	}

	bx::memCopy(ms->m_Ptr, data, size);
	ms->m_Ptr += size;
	ms->m_Size += size;

	return true;
}

bool memStreamRead(MemoryStream* ms, void* data, uint32_t size)
{
	const ptrdiff_t curPos = ms->m_Ptr - ms->m_Data;
	if (curPos + size > ms->m_Size) {
		return false;
	}

	bx::memCopy(data, ms->m_Ptr, size);
	ms->m_Ptr += size;

	return true;
}

#if 0
bool MemoryStream::write_string(const char* str, uint32_t len)
{
	if (!write(&len, sizeof(uint32_t))) {
		return false;
	}

	return write(str, len);
}

bool MemoryStream::read_string(jtl::string& str)
{
	uint32_t len;
	if (!read(&len, sizeof(uint32_t))) {
		return false;
	}

	str.resize(len);
	if (len) {
		if (!read(&str[0], len)) {
			return false;
		}
	}

	return true;
}
#endif

uint8_t* memStreamGetWritePtr(MemoryStream* ms, uint32_t size)
{
	if (ms->m_Size + size > ms->m_Capacity) {
		const uint32_t newCapacity = ms->m_Capacity + bx::uint32_max(MIN_BLOCK_SIZE, size);

		const ptrdiff_t curPos = ms->m_Ptr - ms->m_Data;
		ms->m_Data = (uint8_t*)BX_REALLOC(ms->m_Allocator, ms->m_Data, newCapacity);
		if (!ms->m_Data) {
			return nullptr;
		}

		ms->m_Ptr = ms->m_Data + curPos;
		ms->m_Capacity = newCapacity;
	}

	uint8_t* ptr = ms->m_Ptr;
	ms->m_Ptr += size;
	ms->m_Size += size;

	return ptr;
}

const uint8_t* memStreamGetReadPtr(MemoryStream* ms, uint32_t size)
{
	const ptrdiff_t curPos = ms->m_Ptr - ms->m_Data;
	if (curPos + size > ms->m_Size) {
		return nullptr;
	}

	const uint8_t* ptr = ms->m_Ptr;
	ms->m_Ptr += size;
	return ptr;
}

void memStreamReset(MemoryStream* ms)
{
	ms->m_Ptr = ms->m_Data;
}

bool memStreamEnd(const MemoryStream* ms)
{
	const ptrdiff_t curPos = ms->m_Ptr - ms->m_Data;
	return curPos >= (ptrdiff_t)ms->m_Size;
}

jx::hash128 memStreamCalcHash(const MemoryStream* ms)
{
	jx::hash128 hash;
	jx::spookyHash128(ms->m_Data, ms->m_Size, &hash.m_Value[0], &hash.m_Value[1]);
	return hash;
}
}
