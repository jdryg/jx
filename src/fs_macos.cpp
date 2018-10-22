#include <bx/bx.h>

#if BX_PLATFORM_OSX
#include <jx/fs.h>
#include <jx/allocator.h>
#include <jx/utf8.h>
#include <bx/uint32_t.h>
#include <bx/allocator.h>
#include <bx/string.h>
#include "TargetConditionals.h"
#include <mach-o/dyld.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <sys/stat.h> // S_IRWXU
#include <stdio.h>

namespace jx
{
struct FileSystem
{
	char* m_InstallFolder;
	char* m_UserDataFolder;
};

struct FileFlags
{
	enum Enum : uint32_t
	{
		Read = 1 << 0,
		Write = 1 << 1, 
		Binary = 1 << 2
	};
};

struct File
{
	FILE* m_Handle;
	uint32_t m_Flags;
};

static FileSystem* s_FS = nullptr;

static char* getInstallFolder();
static char* getUserDataFolder(const char* appName);
static bool createDirectory(const char* path);
static bool setCurrentDirectory(BaseDir::Enum baseDir);

bool fsInit(const char* appName)
{
	JX_CHECK(s_FS == nullptr, "FileSystem already initialized");

	const uint32_t totalMem = sizeof(FileSystem);
	uint8_t* mem = (uint8_t*)JX_ALLOC(totalMem);
	if (!mem) {
		return false;
	}

	bx::memSet(mem, 0, totalMem);

	s_FS = (FileSystem*)mem;

	s_FS->m_InstallFolder = getInstallFolder();
	if (!s_FS->m_InstallFolder) {
		return false;
	}

	s_FS->m_UserDataFolder = getUserDataFolder(appName);
	if (!s_FS->m_UserDataFolder) {
		return false;
	}

	// Make sure user data folder exists by trying to create it.
	if (!createDirectory(s_FS->m_UserDataFolder)) {
		return false;
	}

	return true;
}

void fsShutdown()
{
	JX_CHECK(s_FS != nullptr, "FileSystem hasn't been initialized or already deinitialized");

	JX_FREE(s_FS->m_InstallFolder);
	JX_FREE(s_FS->m_UserDataFolder);
	JX_FREE(s_FS);
	s_FS = nullptr;
}

bool fsIsReady()
{
	return s_FS != nullptr;
}

File* fsFileOpenRead(BaseDir::Enum baseDir, const char* relPath)
{
	bool setcwdResult = setCurrentDirectory(baseDir);
	if(!setcwdResult) {
		JX_CHECK(false, "Failed to set current working directory");
		return nullptr;
	}

    FILE* handle = fopen(relPath, "rb");
    if(!handle) {
        return nullptr;
    }

	File* file = (File*)JX_ALLOC(sizeof(File));
	file->m_Handle = handle;
	file->m_Flags = FileFlags::Read | FileFlags::Binary;
	return file;
}

File* fsFileOpenWrite(BaseDir::Enum baseDir, const char* relPath)
{
	if (baseDir != BaseDir::UserData) {
		JX_CHECK(false, "Can only write to user data folder");
		return nullptr;
	}

	if (!setCurrentDirectory(BaseDir::UserData)) {
		JX_CHECK(false, "Failed to set current working directory");
		return nullptr;
	}

    FILE* handle = fopen(relPath, "wb");
    if(!handle) {
        return nullptr;
    }

	File* file = (File*)JX_ALLOC(sizeof(File));
	file->m_Handle = handle;
	file->m_Flags = FileFlags::Write | FileFlags::Binary;
	return file;
}

void fsFileClose(File* f)
{
	JX_CHECK(f != nullptr, "Cannot close NULL file");
	fclose(f->m_Handle);
	JX_FREE(f);
}

uint32_t fsFileReadBytes(File* f, void* buffer, uint32_t len)
{
	JX_CHECK(f != nullptr && f->m_Handle != nullptr, "Trying to read from a null file");
	JX_CHECK((f->m_Flags & FileFlags::Read) != 0, "Trying to read bytes from a file opened for writing");

    return fread(buffer, 1, len, f->m_Handle);
}

uint32_t fsFileWriteBytes(File* f, const void* buffer, uint32_t len)
{
	JX_CHECK(f != nullptr && f->m_Handle != nullptr, "Trying to write from a null file");
	JX_CHECK((f->m_Flags & FileFlags::Write) != 0, "Trying to write bytes to a file opened for reading");

    return fwrite(buffer, 1, len, f->m_Handle);
}

uint64_t fsFileGetSize(File* f)
{
	JX_CHECK(f != nullptr && f->m_Handle != nullptr, "Trying to read from a null file");
	JX_CHECK((f->m_Flags & FileFlags::Read) != 0, "Cannot get the size of a file opened for writing");

    const long prevPos = ftell(f->m_Handle);
    fseek(f->m_Handle, 0, SEEK_END);
    const long size = ftell(f->m_Handle);
    fseek(f->m_Handle, prevPos, SEEK_SET);
    return (uint64_t)size;
}

void fsFileSeek(File* f, int offset, SeekOrigin::Enum origin)
{
	JX_CHECK(f != nullptr && f->m_Handle != nullptr, "Trying to read from a null file");
	
    fseek(f->m_Handle, (long)offset, origin == SeekOrigin::Begin ? SEEK_SET : origin == SeekOrigin::Current ? SEEK_CUR : SEEK_END);
}

bool fsFileRemove(BaseDir::Enum baseDir, const char* relPath)
{
	if (baseDir != BaseDir::UserData) {
		JX_CHECK(false, "Can only remove files from user data folder");
		return false;
	}

	if (!setCurrentDirectory(BaseDir::UserData)) {
		JX_CHECK(false, "Failed to set current working directory");
		return false;
	}

    return remove(relPath) == 0;
}

bool fsCreateFolderTree(BaseDir::Enum baseDir, const char* relPath)
{
	if (baseDir != BaseDir::UserData) {
		JX_CHECK(false, "Can only create subfolders inside user data folder");
		return false;
	}

	if (!setCurrentDirectory(BaseDir::UserData)) {
		JX_CHECK(false, "Failed to set current working directory");;
		return false;
	}

	char partialPath[512];

	const char* slash = bx::strFind(relPath, '/');
	while (slash) {
		const uint32_t partialLen = (uint32_t)(slash - relPath);
		JX_CHECK(partialLen < BX_COUNTOF(partialPath) - 1, "Partial path too long!");

		const uint32_t copyLen = bx::uint32_min(partialLen, BX_COUNTOF(partialPath) - 1);
		bx::memCopy(partialPath, relPath, sizeof(char) * copyLen);
		partialPath[copyLen] = '\0';

		if (!createDirectory(partialPath)) {
			return false;
		}

		slash = bx::strFind(slash + 1, '/');
	}

	return createDirectory(relPath);
}

bool fsEnumerateFiles(BaseDir::Enum baseDir, const char* relPath, EnumerateFilesCallback callback)
{
	const bool setcwdResult = setCurrentDirectory(baseDir);
	if (!setcwdResult) {
		JX_CHECK(false, "Failed to set current working directory");
		return false;
	}

	const char* lastSlash = bx::strRFind(relPath, '/');
	if(!lastSlash) {
		return false;
	}
	const uint32_t len = (uint32_t)(lastSlash - relPath);

	char folder[256];
	bx::memCopy(folder, relPath, len);
	folder[len] = '\0';

	DIR* d = opendir(folder);
	if(d) {
		struct dirent* dir = nullptr;
		while((dir = readdir(d)) != nullptr) {
			if(dir->d_type == DT_REG) {
				callback(dir->d_name, true);
			}
		}
	}

	return true;
}

const char* fsGetBaseDirPath(BaseDir::Enum baseDir)
{
	switch (baseDir) {
	case BaseDir::Install:  return s_FS->m_InstallFolder;
	case BaseDir::UserData: return s_FS->m_UserDataFolder;
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
// Internal
//
static char* getInstallFolder()
{
	char folder[512];
	uint32_t size = BX_COUNTOF(folder);
	if(_NSGetExecutablePath(folder, &size)) {
		return nullptr;
	}

	bx::StringView sv(folder, size);
	const char* lastSlash = bx::strRFind(sv, '/');
	if(!lastSlash) {
		return nullptr;
	}
	
	const uint32_t len = (uint32_t)(lastSlash - folder);
	char* installFolder = (char*)JX_ALLOC(len + 1);
	bx::memCopy(installFolder, folder, len);
	installFolder[len] = '\0';

    return installFolder;
}

static char* getUserDataFolder(const char* appName)
{
	const char* homedir = nullptr;
	if((homedir = getenv("HOME")) == nullptr) {
		homedir = getpwuid(getuid())->pw_dir;
	}

	if(!homedir) {
		return nullptr;
	}

	char docPath[512];
	bx::snprintf(docPath, BX_COUNTOF(docPath), "%s/Documents/%s/", homedir, appName);

	const uint32_t len = (uint32_t)bx::strLen(docPath);
	char* dataFolder = (char*)JX_ALLOC(len + 1);
	bx::memCopy(dataFolder, docPath, len);
	dataFolder[len] = '\0';

	return dataFolder;
}

static bool setCurrentDirectory(BaseDir::Enum baseDir)
{
	const char* dir = nullptr;
	switch (baseDir) {
	case BaseDir::Install:
		dir = s_FS->m_InstallFolder;
		break;
	case BaseDir::UserData:
		dir = s_FS->m_UserDataFolder;
		break;
	default:
		JX_CHECK(false, "Unknown base dir");
		return false;
	}

    return chdir(dir) == 0;
}

static bool createDirectory(const char* path)
{
	if (!path) {
		return false;
	}

	mkdir(path, S_IRWXU);

	return true;
}
}
#endif
