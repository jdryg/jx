#ifndef JX_FS_H
#error "Must be included from jx/fs.h"
#endif

namespace jx
{
template<typename T>
inline T fsFileRead(File* f)
{
	T val;
	fsFileReadBytes(f, &val, sizeof(T));
	return val;
}

template<>
inline jtl::string fsFileRead(File* f)
{
	const uint32_t len = fsFileRead<uint32_t>(f);
	jtl::string str;
	str.resize(len);

	if (len) {
		fsFileReadBytes(f, &str[0], len);
	}

	return str;
}

template<typename T>
inline bool fsFileWrite(File* f, T val)
{
	return fsFileWriteBytes(f, &val, sizeof(T)) == sizeof(T);
}

inline bool fsFileWrite(File* f, const char* str)
{
	const uint32_t len = bx::strLen(str);
	if (fsFileWrite<uint32_t>(f, len)) {
		return false;
	}

	return fsFileWriteBytes(f, str, len) == len;
}
}