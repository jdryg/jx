#ifndef JX_FS_H
#define JX_FS_H

#include <stdint.h>
#include <bx/platform.h>

namespace bx
{
struct AllocatorI;
}

namespace jx
{
struct BaseDir
{
	enum Enum
	{
		Install,
		UserData,
		UserAppData,
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

struct FileSystemFlags
{
	enum Enum : uint32_t
	{
		None = 0,
		AllowWritingToInstallDir = 1u << 0
	};
};

struct FileTime
{
	uint16_t m_Year;
	uint16_t m_Month;
	uint16_t m_Day;
	uint16_t m_Hour;
	uint16_t m_Minute;
	uint16_t m_Second;
	uint16_t m_Millisecond;
};

struct FileTimeType
{
	enum Enum : uint32_t
	{
		Creation,
		LastAccess,
		LastWrite
	};
};

struct File;

typedef void (*EnumFilesCallback)(const char* relPath, bool isFile, void* userData);

bool fsInit(const char* appName, uint32_t flags);
void fsShutdown();
#if BX_PLATFORM_EMSCRIPTEN
bool fsIsReady();
void fsSync();
#endif

#if BX_PLATFORM_WINDOWS
const wchar_t* fsGetBaseDirPath(BaseDir::Enum baseDir);
#endif

bool fsSetUserDataFolder(const char* absFolderPath);

File* fsFileOpenRead(BaseDir::Enum baseDir, const char* relPath);
File* fsFileOpenWrite(BaseDir::Enum baseDir, const char* relPath);
void fsFileClose(File* f);
uint32_t fsFileReadBytes(File* f, void* buffer, uint32_t len);
uint32_t fsFileWriteBytes(File* f, const void* buffer, uint32_t len);
uint64_t fsFileGetSize(File* f);
void fsFileSeek(File* f, int64_t offset, SeekOrigin::Enum origin);
int64_t fsFileTell(File* f);
bool fsFileGetTime(File* f, jx::FileTimeType::Enum type, FileTime* t);

bool fsRemoveFile(BaseDir::Enum baseDir, const char* relPath);
bool fsCopyFile(BaseDir::Enum srcBaseDir, const char* srcPath, BaseDir::Enum dstBaseDir, const char* dstPath);
bool fsMoveFile(BaseDir::Enum srcBaseDir, const char* srcPath, BaseDir::Enum dstBaseDir, const char* dstPath);

bool fsCreateFolderTree(BaseDir::Enum baseDir, const char* relPath);
bool fsRemoveEmptyFolder(BaseDir::Enum baseDir, const char* relPath);

bool fsEnumerateFiles(BaseDir::Enum baseDir, const char* relPath, EnumFilesCallback callback, void* userData);

void fsConvertStringToFilename(const char* name, char* filename, uint32_t maxLen);

void fsSplitPath(const char* path, char* drive, char* dir, char* filename, char* ext);

char* fsLoadTextFile(BaseDir::Enum baseDir, const char* relPath, uint64_t* size, bx::AllocatorI* allocator);
}

#include "inline/fs.inl"

#endif
