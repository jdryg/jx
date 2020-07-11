#include <jx/json_writer.h>
#include <jtl/string.h>

namespace jx
{
struct JSONWriter
{
	bx::AllocatorI* m_Allocator;
	jtl::string m_JSON;
	int m_Level;
	int m_ArrayLevel;
	bool m_InArray;
	bool m_AddComma;
};

static jtl::string escapeString(const char* str);

JSONWriter* createJSONWriter(bx::AllocatorI* allocator)
{
	JSONWriter* json = BX_NEW(allocator, JSONWriter)();
	if (!json) {
		return nullptr;
	}

	json->m_Allocator = allocator;
	json->m_Level = 0;
	json->m_ArrayLevel = 0;
	json->m_InArray = false;
	json->m_AddComma = false;
	json->m_JSON.reserve(64 * 1024);

	return json;
}

void destroyJSONWriter(JSONWriter* writer)
{
	bx::AllocatorI* allocator = writer->m_Allocator;

	BX_DELETE(allocator, writer);
}

void jsonWriteJSON(JSONWriter* writer, const char* name, const JSONWriter* json)
{
	if (writer->m_AddComma) {
		writer->m_JSON.append(",");
	}

	writer->m_JSON.append("\"");
	writer->m_JSON.append(name);
	writer->m_JSON.append("\":");
	writer->m_JSON.append(json->m_JSON.c_str());

	writer->m_AddComma = true;
}

void jsonWriteString(JSONWriter* writer, const char* name, const char* value)
{
	if (writer->m_AddComma) {
		writer->m_JSON.append(",");
	}

	if (name) {
		writer->m_JSON.append("\"");
		writer->m_JSON.append(name);
		writer->m_JSON.append("\":\"");
	} else {
		writer->m_JSON.append("\"");
	}
	writer->m_JSON.append(escapeString(value).c_str());
	writer->m_JSON.append("\"");

	writer->m_AddComma = true;
}

void jsonWriteInt(JSONWriter* writer, const char* name, int value)
{
	if (writer->m_AddComma) {
		writer->m_JSON.append(",");
	}

	writer->m_JSON.append("\"");
	writer->m_JSON.append(name);
	writer->m_JSON.append("\":");
	writer->m_JSON.append(jtl::to_string(value).c_str());

	writer->m_AddComma = true;
}

void jsonWriteUInt(JSONWriter* writer, const char* name, uint32_t value)
{
	if (writer->m_AddComma) {
		writer->m_JSON.append(",");
	}

	if (name) {
		writer->m_JSON.append("\"");
		writer->m_JSON.append(name);
		writer->m_JSON.append("\":");
	}
	writer->m_JSON.append(jtl::to_string(value).c_str());

	writer->m_AddComma = true;
}

void jsonWriteBoolean(JSONWriter* writer, const char* name, bool value)
{
	if (writer->m_AddComma) {
		writer->m_JSON.append(",");
	}

	if (name) {
		writer->m_JSON.append("\"");
		writer->m_JSON.append(name);
		writer->m_JSON.append("\":");
	}
	if (value) {
		writer->m_JSON.append("true");
	} else {
		writer->m_JSON.append("false");
	}

	writer->m_AddComma = true;
}

void jsonWriteFloat(JSONWriter* writer, const char* name, float value)
{
	if (writer->m_AddComma) {
		writer->m_JSON.append(",");
	}

	if (name) {
		writer->m_JSON.append("\"");
		writer->m_JSON.append(name);
		writer->m_JSON.append("\":");
	}
	writer->m_JSON.append(jtl::to_string(value).c_str());

	writer->m_AddComma = true;
}

void jsonBeginObject(JSONWriter* writer, const char* name)
{
	if (writer->m_AddComma) {
		writer->m_JSON.append(",");
	}

	if (name) {
		writer->m_JSON.append("\"");
		writer->m_JSON.append(name);
		writer->m_JSON.append("\":{");
	} else {
		writer->m_JSON.append("{");
	}

	writer->m_AddComma = false;
	++writer->m_Level;
}

void jsonEndObject(JSONWriter* writer)
{
	writer->m_JSON.append("}");
	--writer->m_Level;
	writer->m_AddComma = true;
}

void jsonBeginArray(JSONWriter* writer, const char* name)
{
	if (writer->m_AddComma) {
		writer->m_JSON.append(",");
	}

	if (name) {
		writer->m_JSON.append("\"");
		writer->m_JSON.append(name);
		writer->m_JSON.append("\":[");
	} else {
		writer->m_JSON.append("[");
	}

	++writer->m_Level;
	writer->m_ArrayLevel = writer->m_Level;
	writer->m_InArray = true;
	writer->m_AddComma = false;
}

void jsonEndArray(JSONWriter* writer)
{
	writer->m_JSON.append("]");
	--writer->m_Level;
	writer->m_ArrayLevel = -1;
	writer->m_InArray = false;
	writer->m_AddComma = true;
}

const char* jsonGetString(const JSONWriter* writer, uint32_t* sz)
{
	*sz = writer->m_JSON.size();
	return writer->m_JSON.c_str();
}

static jtl::string escapeString(const char* str)
{
	jtl::string s(str);

	// TODO: Escape illegal characters, like double quotes and backward slashes.

	return s;
}
}
