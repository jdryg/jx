#ifndef JX_BASE64_H
#define JX_BASE64_H

#include <stdint.h>

namespace bx
{
struct AllocatorI;
}

namespace jx
{
uint8_t* base64Encode(const uint8_t* src, size_t len, size_t* out_len, bx::AllocatorI* allocator);
uint8_t* base64Decode(const uint8_t* src, size_t len, size_t* out_len, bx::AllocatorI* allocator);
}

#endif // JX_BASE64_H