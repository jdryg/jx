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
		UserData
	};
};

struct File;

typedef jtl::delegate<void(const char* relPath, bool isFile)> EnumerateFilesCallback;

bool fsInit(const char* appName);
void fsShutdown();

File* fsFileOpenRead(BaseDir::Enum baseDir, const char* relPath);
File* fsFileOpenWrite(BaseDir::Enum baseDir, const char* relPath);
void fsFileClose(File* f);
uint32_t fsFileReadBytes(File* f, void* buffer, uint32_t len);
uint32_t fsFileWriteBytes(File* f, const void* buffer, uint32_t len);
uint64_t fsFileGetSize(File* f);

bool fsFileRemove(BaseDir::Enum baseDir, const char* relPath);

bool fsCreateFolderTree(BaseDir::Enum baseDir, const char* relPath);

bool fsEnumerateFiles(BaseDir::Enum baseDir, const char* relPath, EnumerateFilesCallback callback);

void fsConvertStringToFilename(const char* name, char* filename, uint32_t maxLen);

template<typename T> T fsFileRead(File* f);
template<> jtl::string fsFileRead(File* f);
template<typename T> bool fsFileWrite(File* f, T val);
bool fsFileWrite(File* f, const char* str);

void fsSplitPath(const char* path, char* drive, char* dir, char* filename, char* ext);
}

#include "inline/fs.inl"

#endif
