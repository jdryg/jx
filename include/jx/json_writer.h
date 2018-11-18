#ifndef JX_JSON_WRITER_H
#define JX_JSON_WRITER_H

#include <stdint.h>

namespace bx
{
struct AllocatorI;
}

namespace jx
{
struct JSONWriter;

JSONWriter* createJSONWriter(bx::AllocatorI* allocator);
void destroyJSONWriter(JSONWriter* writer);

void jsonWriteJSON(JSONWriter* writer, const char* name, const JSONWriter* json);
void jsonWriteString(JSONWriter* writer, const char* name, const char* value);
//void jsonWriteString(JSONWriter* writer, const char* name, const jtl::string& str);
void jsonWriteInt(JSONWriter* writer, const char* name, int value);
void jsonWriteUInt(JSONWriter* writer, const char* name, uint32_t value);
void jsonWriteBoolean(JSONWriter* writer, const char* name, bool value);
void jsonWriteFloat(JSONWriter* writer, const char* name, float value);

void jsonBeginObject(JSONWriter* writer, const char* name);
void jsonEndObject(JSONWriter* writer);

void jsonBeginArray(JSONWriter* writer, const char* name);
void jsonEndArray(JSONWriter* writer);

const char* jsonGetString(const JSONWriter* writer, uint32_t* sz);
}

#endif
