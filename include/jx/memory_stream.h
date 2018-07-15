#ifndef JX_MEMORY_STREAM_H
#define JX_MEMORY_STREAM_H

#include <stdint.h>
#include <jx/hash128.h>

namespace bx
{
struct AllocatorI;
}

namespace jx
{
struct MemoryStream;

MemoryStream* createMemoryStream(bx::AllocatorI* allocator);
void destroyMemoryStream(MemoryStream* ms);

bool memStreamCopy(MemoryStream* dst, const MemoryStream* src);
bool memStreamWrite(MemoryStream* ms, const void* data, uint32_t size);
bool memStreamRead(MemoryStream* ms, void* data, uint32_t size);
uint8_t* memStreamGetWritePtr(MemoryStream* ms, uint32_t size);
const uint8_t* memStreamGetReadPtr(MemoryStream* ms, uint32_t size);
void memStreamReset(MemoryStream* ms);
bool memStreamEnd(const MemoryStream* ms);
jx::hash128 memStreamCalcHash(const MemoryStream* ms);

uint8_t memStreamRead_uint8(MemoryStream* ms);
uint32_t memStreamRead_uint32(MemoryStream* ms);
bool memStreamRead_string(MemoryStream* ms, char* str, uint32_t maxLen);

bool memStreamWrite_uint8(MemoryStream* ms, uint8_t val);
bool memStreamWrite_uint32(MemoryStream* ms, uint32_t val);
bool memStreamWrite_string(MemoryStream* ms, const char* str);

uint32_t memStreamGetSize(const MemoryStream* ms);
}

#include "inline/memory_stream.inl"

#endif
