#ifndef JX_BSON_H
#define JX_BSON_H

#include <stdint.h>

namespace bx
{
struct AllocatorI;
}

namespace jx
{
struct BSONReader;

BSONReader* bsonReaderCreate(const uint8_t* buffer, uint32_t sz, bx::AllocatorI* allocator);
void bsonReaderDestroy(BSONReader* bson, bx::AllocatorI* allocator);

bool bsonReaderGetInt32(const BSONReader* bson, const char* key, int32_t* value);
bool bsonReaderGetInt64(const BSONReader* bson, const char* key, int64_t* value);
bool bsonReaderGetTimestamp(const BSONReader* bson, const char* key, uint64_t* value);
bool bsonReaderGetUTF8String(const BSONReader* bson, const char* key, char* value, uint32_t sz);
BSONReader* bsonReaderGetArray(BSONReader* bson, const char* key, bx::AllocatorI* allocator);
BSONReader* bsonReaderGetDocument(BSONReader* bson, const char* key, bx::AllocatorI* allocator);
bool bsonReaderGetBinary(const BSONReader* bson, const char* key, uint8_t** data, uint32_t* sz, bx::AllocatorI* allocator);
}

#endif
