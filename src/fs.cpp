#include <jx/fs.h>
#include <bx/platform.h>

namespace jx
{
void fsSplitPath(const char* path, char* drive, char* directory, char* filename, char* extension)
{
#if BX_PLATFORM_WINDOWS
	_splitpath(path, drive, directory, filename, extension);
#elif BX_PLATFORM_LINUX || BX_PLATFORM_OSX || BX_PLATFORM_RPI
	if (drive) {
		*drive = '\0';
	}

	if (directory) {
		const char* lastSlash = strrchr(path, '/');
		if (lastSlash) {
			size_t len = (size_t)(lastSlash - path);
			memcpy(directory, path, len);
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
			memcpy(filename, lastSlash, len);
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
			memcpy(extension, dot, len);
			extension[len] = '\0';
		} else {
			*extension = '\0';
		}
	}
#endif
}
}
