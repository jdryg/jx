#include <jx/bson.h>
#include <bx/allocator.h>
#include <bx/string.h>

namespace jx
{
static const uint8_t kTypeDouble = 0x01;
static const uint8_t kTypeUTF8String = 0x02;
static const uint8_t kTypeDocument = 0x03;
static const uint8_t kTypeArray = 0x04;
static const uint8_t kTypeBinary = 0x05;
static const uint8_t kTypeBoolean = 0x08;
static const uint8_t kTypeNull = 0x0A;
static const uint8_t kTypeInt32 = 0x10;
static const uint8_t kTypeTimestamp = 0x11;
static const uint8_t kTypeInt64 = 0x12;

struct BSONReader
{
	const uint8_t* m_Buffer;
	uint32_t m_Size;
};

static const uint8_t* bsonReader_findKey(const BSONReader* bson, const char* key, const uint8_t** valuePtr);

BSONReader* bsonReaderCreate(const uint8_t* buffer, uint32_t sz, bx::AllocatorI* allocator)
{
	BSONReader* bson = (BSONReader*)BX_ALLOC(allocator, sizeof(BSONReader));
	if (bson == nullptr) {
		return nullptr;
	}

	bx::memSet(bson, 0, sizeof(BSONReader));
	bson->m_Buffer = buffer;
	bson->m_Size = sz;

	return bson;
}

void bsonReaderDestroy(BSONReader* bson, bx::AllocatorI* allocator)
{
	BX_FREE(allocator, bson);
}

bool bsonReaderGetInt32(const BSONReader* bson, const char* key, int32_t* value)
{
	const uint8_t* valuePtr = nullptr;
	const uint8_t* obj = bsonReader_findKey(bson, key, &valuePtr);
	if (obj == nullptr || *obj != kTypeInt32) {
		return false;
	}

	bx::memCopy(value, valuePtr, sizeof(int32_t));

	return true;
}

bool bsonReaderGetInt64(const BSONReader* bson, const char* key, int64_t* value)
{
	const uint8_t* valuePtr = nullptr;
	const uint8_t* obj = bsonReader_findKey(bson, key, &valuePtr);
	if (obj == nullptr || *obj != kTypeInt64) {
		return false;
	}

	bx::memCopy(value, valuePtr, sizeof(int64_t));

	return true;
}

bool bsonReaderGetTimestamp(const BSONReader* bson, const char* key, uint64_t* value)
{
	const uint8_t* valuePtr = nullptr;
	const uint8_t* obj = bsonReader_findKey(bson, key, &valuePtr);
	if (obj == nullptr || *obj != kTypeTimestamp) {
		return false;
	}

	bx::memCopy(value, valuePtr, sizeof(uint64_t));

	return true;
}

bool bsonReaderGetUTF8String(const BSONReader* bson, const char* key, char* value, uint32_t sz)
{
	const uint8_t* valuePtr = nullptr;
	const uint8_t* obj = bsonReader_findKey(bson, key, &valuePtr);
	if (obj == nullptr || *obj != kTypeUTF8String) {
		return false;
	}

	const uint32_t strSize = *(uint32_t*)valuePtr;
	bx::memCopy(value, valuePtr + sizeof(int32_t), bx::min<uint32_t>(sz, strSize));

	return true;
}

BSONReader* bsonReaderGetArray(BSONReader* bson, const char* key, bx::AllocatorI* allocator)
{
	const uint8_t* valuePtr = nullptr;
	const uint8_t* obj = bsonReader_findKey(bson, key, &valuePtr);
	if (obj == nullptr || *obj != kTypeArray) {
		return nullptr;
	}

	return bsonReaderCreate(valuePtr, *(uint32_t*)valuePtr, allocator);
}

BSONReader* bsonReaderGetDocument(BSONReader* bson, const char* key, bx::AllocatorI* allocator)
{
	const uint8_t* valuePtr = nullptr;
	const uint8_t* obj = bsonReader_findKey(bson, key, &valuePtr);
	if (obj == nullptr || *obj != kTypeDocument) {
		return nullptr;
	}

	return bsonReaderCreate(valuePtr, *(uint32_t*)valuePtr, allocator);
}

bool bsonReaderGetBinary(const BSONReader* bson, const char* key, uint8_t** data, uint32_t* sz, bx::AllocatorI* allocator)
{
	const uint8_t* valuePtr = nullptr;
	const uint8_t* obj = bsonReader_findKey(bson, key, &valuePtr);
	if (obj == nullptr || *obj != kTypeBinary) {
		return false;
	}

	const uint32_t size = *(uint32_t*)valuePtr;
	const uint8_t subtype = valuePtr[sizeof(uint32_t)];

	*data = (uint8_t*)BX_ALLOC(allocator, size);
	if (*data == nullptr) {
		return false;
	}

	bx::memCopy(*data, valuePtr + sizeof(uint32_t) + sizeof(uint8_t), size);
	*sz = size;

	return true;
}

//////////////////////////////////////////////////////////////////////////
// Internal API
//
static const uint8_t* bsonReader_findKey(const BSONReader* bson, const char* key, const uint8_t** valuePtr)
{
	bx::StringView svKey(key);

	const uint8_t* ptr = &bson->m_Buffer[4];
	while (*ptr != '\0') {
		// Get element type
		const uint8_t* typePtr = ptr++;

		const bool match = bx::strCmp((const char*)ptr, svKey) == 0;

		// Skip element name
		while (*ptr != '\0') {
			ptr++;
		}
		ptr++;

		if (match) {
			*valuePtr = ptr;
			return typePtr;
		}

		// Skip value
		switch (*typePtr) {
		case kTypeDouble:
			ptr += sizeof(double);
			break;
		case kTypeBoolean:
			ptr += sizeof(bool);
			break;
		case kTypeDocument:
		case kTypeArray:
			ptr += *(int32_t*)ptr;
			break;
		case kTypeInt32:
			ptr += sizeof(int32_t);
			break;
		case kTypeInt64:
			ptr += sizeof(int64_t);
			break;
		case kTypeNull:
			break;
		case kTypeTimestamp:
			ptr += sizeof(uint64_t);
			break;
		case kTypeUTF8String:
			ptr += *(int32_t*)ptr + sizeof(int32_t);
			break;
		default:
			return nullptr;
		}
	}

	return nullptr;
}
}
