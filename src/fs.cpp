#include <jx/fs.h>
#include <bx/platform.h>
#include <bx/uint32_t.h>

#if BX_PLATFORM_LINUX || BX_PLATFORM_OSX || BX_PLATFORM_RPI || BX_PLATFORM_EMSCRIPTEN
#include <string.h>
#endif

namespace jx
{
void fsSplitPath(const char* path, char* drive, char* directory, char* filename, char* extension)
{
#if BX_PLATFORM_WINDOWS
	_splitpath(path, drive, directory, filename, extension);
#elif BX_PLATFORM_LINUX || BX_PLATFORM_OSX || BX_PLATFORM_RPI || BX_PLATFORM_EMSCRIPTEN
	if (drive) {
		*drive = '\0';
	}

	if (directory) {
		const char* lastSlash = strrchr(path, '/');
		if (lastSlash) {
			size_t len = (size_t)(lastSlash - path);
			bx::memCopy(directory, path, len);
			directory[len] = '\0';
		} else {
			*directory = '\0';
		}
	}

	if (filename) {
		const char* lastSlash = strrchr(path, '/');
		if (!lastSlash) {
			lastSlash = path;
		} else {
			++lastSlash;
		}

		const char* dot = strrchr(path, '.');
		if (dot && dot > lastSlash) {
			size_t len = (size_t)(dot - lastSlash);
			bx::memCopy(filename, lastSlash, len);
			filename[len] = '\0';
		} else {
			*filename = '\0';
		}
	}

	if (extension) {
		const char* lastSlash = strrchr(path, '/');
		const char* dot = strrchr(path, '.');
		if (dot && dot > lastSlash) {
			size_t len = (size_t)(strlen(path) - strlen(dot));
			bx::memCopy(extension, dot, len);
			extension[len] = '\0';
		} else {
			*extension = '\0';
		}
	}
#else
#error "Not implemented"
#endif
}

void fsConvertStringToFilename(const char* name, char* filename, uint32_t maxLen)
{
	const bx::StringView svIllegalChars("\\/:*?\"<>|");

	const uint32_t nameLen = (uint32_t)bx::strLen(name);
	const uint32_t len = bx::uint32_min(nameLen, maxLen - 1);

	char* dst = filename;
	for (uint32_t i = 0; i < nameLen; ++i) {
		if (bx::strFind(svIllegalChars, name[i]) == nullptr) {
			*dst++ = name[i];
		} else {
			*dst++ = '_';
		}
	}
	*dst = '\0';
}

char* fsLoadTextFile(BaseDir::Enum baseDir, const char* relPath, bx::AllocatorI* allocator)
{
	File* f = fsFileOpenRead(baseDir, relPath);
	if (!f) {
		return nullptr;
	}

	const uint64_t size = fsFileGetSize(f);
	char* buffer = (char*)BX_ALLOC(allocator, size + 1);
	if (!buffer) {
		fsFileClose(f);
		return nullptr;
	}
	fsFileReadBytes(f, buffer, (uint32_t)size);
	buffer[size] = '\0';

	fsFileClose(f);

	return buffer;
}
}
