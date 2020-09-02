#ifndef JX_FS_H
#define JX_FS_H

#include <stdint.h>
#include <jtl/delegate.h>
#include <jtl/string.h> // fsFileRead<jtl::string>()

namespace jx
{
struct BaseDir
{
	enum Enum
	{
		Install,
		UserData,
		AbsolutePath,
	};
};

struct SeekOrigin
{
	enum Enum
	{
		Begin,
		Current,
		End
	};
};

struct File;

typedef jtl::delegate<void(const char* relPath, bool isFile)> EnumerateFilesCallback;

bool fsInit(const char* appName);
void fsShutdown();
#if BX_PLATFORM_EMSCRIPTEN
bool fsIsReady();
void fsSync();
#endif

#if BX_PLATFORM_WINDOWS
const wchar_t* fsGetBaseDirPath(BaseDir::Enum baseDir);
#endif

File* fsFileOpenRead(BaseDir::Enum baseDir, const char* relPath);
File* fsFileOpenWrite(BaseDir::Enum baseDir, const char* relPath);
void fsFileClose(File* f);
uint32_t fsFileReadBytes(File* f, void* buffer, uint32_t len);
uint32_t fsFileWriteBytes(File* f, const void* buffer, uint32_t len);
uint64_t fsFileGetSize(File* f);
void fsFileSeek(File* f, int64_t offset, SeekOrigin::Enum origin);
int64_t fsFileTell(File* f);

bool fsFileRemove(BaseDir::Enum baseDir, const char* relPath);

bool fsCreateFolderTree(BaseDir::Enum baseDir, const char* relPath);
bool fsRemoveEmptyFolder(BaseDir::Enum baseDir, const char* relPath);

bool fsEnumerateFiles(BaseDir::Enum baseDir, const char* relPath, EnumerateFilesCallback callback);

void fsConvertStringToFilename(const char* name, char* filename, uint32_t maxLen);

void fsFileReadString(File* f, char* str, uint32_t maxLen);
bool fsFileWriteString(File* f, const char* str);

template<typename T> T fsFileRead(File* f);
template<> jtl::string fsFileRead(File* f);
template<typename T> bool fsFileWrite(File* f, T val);
bool fsFileWrite(File* f, const char* str);

void fsSplitPath(const char* path, char* drive, char* dir, char* filename, char* ext);

char* fsLoadTextFile(BaseDir::Enum baseDir, const char* relPath, uint64_t* size, bx::AllocatorI* allocator);
}

#include "inline/fs.inl"

#endif
