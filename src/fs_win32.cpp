#include <bx/bx.h>

#if BX_PLATFORM_WINDOWS
#include <jx/fs.h>
#include <jx/allocator.h>
#include <jx/utf8.h>
#include <bx/uint32_t.h>
#include <bx/allocator.h>

#include <Windows.h>
#include <Shlobj.h>

namespace jx
{
struct FileSystem
{
	wchar_t* m_InstallFolder;
	wchar_t* m_UserDataFolder;
	wchar_t* m_UserAppDataFolder;
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
	HANDLE m_Handle;
	uint32_t m_Flags;
};

static FileSystem* s_FS = nullptr;

static wchar_t* getInstallFolder();
static wchar_t* getUserDataFolder(const char* appName);
static wchar_t* getAppDataFolder(const char* appName);
static bool createDirectory(const wchar_t* path);
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

	if (appName != nullptr) {
		// User data folder
		{
			s_FS->m_UserDataFolder = getUserDataFolder(appName);
			if (!s_FS->m_UserDataFolder) {
				return false;
			}

			// Make sure user data folder exists by trying to create it.
			if (!createDirectory(s_FS->m_UserDataFolder)) {
				return false;
			}
		}

		// App data folder
		{
			s_FS->m_UserAppDataFolder = getAppDataFolder(appName);
			if (!s_FS->m_UserAppDataFolder) {
				return false;
			}

			if (!createDirectory(s_FS->m_UserAppDataFolder)) {
				return false;
			}
		}
	}

	return true;
}

void fsShutdown()
{
	JX_CHECK(s_FS != nullptr, "FileSystem hasn't been initialized or already deinitialized");

	JX_FREE(s_FS->m_InstallFolder);
	JX_FREE(s_FS->m_UserDataFolder);
	JX_FREE(s_FS->m_UserAppDataFolder);
	JX_FREE(s_FS);
	s_FS = nullptr;
}

bool fsIsReady()
{
	return s_FS != nullptr;
}

bool fsSetUserDataFolder(const char* absFolderPath)
{
	wchar_t utf16AbsPath[256];
	utf8ToUtf16(absFolderPath, (uint16_t*)utf16AbsPath, BX_COUNTOF(utf16AbsPath));

	const uint32_t len = (uint32_t)wcslen(utf16AbsPath);
	wchar_t* str = (wchar_t*)JX_ALLOC(sizeof(wchar_t) * (len + 1));
	if (!str) {
		return false;
	}

	bx::memCopy(str, utf16AbsPath, sizeof(wchar_t) * len);
	str[len] = L'\0';
	
	JX_FREE(s_FS->m_UserDataFolder);
	s_FS->m_UserDataFolder = str;

	return true;
}

File* fsFileOpenRead(BaseDir::Enum baseDir, const char* relPath)
{
	bool setcwdResult = setCurrentDirectory(baseDir);
	if(!setcwdResult) {
		JX_CHECK(false, "Failed to set current working directory");
		return nullptr;
	}

	wchar_t utf16RelPath[512];
	utf8ToUtf16(relPath, (uint16_t*)&utf16RelPath[0], BX_COUNTOF(utf16RelPath));

	HANDLE handle = ::CreateFileW(utf16RelPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (handle == INVALID_HANDLE_VALUE) {
		return nullptr;
	}

	File* file = (File*)JX_ALLOC(sizeof(File));
	file->m_Handle = handle;
	file->m_Flags = FileFlags::Read | FileFlags::Binary;
	return file;
}

File* fsFileOpenWrite(BaseDir::Enum baseDir, const char* relPath)
{
	if (baseDir == BaseDir::Install) {
		JX_CHECK(false, "Cannot write in installation directory");
		return nullptr;
	}

	if (!setCurrentDirectory(baseDir)) {
		JX_CHECK(false, "Failed to set current working directory");
		return nullptr;
	}

	wchar_t utf16RelPath[512];
	utf8ToUtf16(relPath, (uint16_t*)&utf16RelPath[0], BX_COUNTOF(utf16RelPath));

	HANDLE handle = ::CreateFileW(utf16RelPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (handle == INVALID_HANDLE_VALUE) {
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
	::CloseHandle(f->m_Handle);
	JX_FREE(f);
}

uint32_t fsFileReadBytes(File* f, void* buffer, uint32_t len)
{
	JX_CHECK(f != nullptr && f->m_Handle != INVALID_HANDLE_VALUE, "Trying to read from a null file");
	JX_CHECK((f->m_Flags & FileFlags::Read) != 0, "Trying to read bytes from a file opened for writing");

	DWORD numBytesRead = 0;
	if (::ReadFile(f->m_Handle, buffer, len, &numBytesRead, NULL) == 0) {
		return 0;
	}

	return (uint32_t)numBytesRead;
}

uint32_t fsFileWriteBytes(File* f, const void* buffer, uint32_t len)
{
	JX_CHECK(f != nullptr && f->m_Handle != INVALID_HANDLE_VALUE, "Trying to write from a null file");
	JX_CHECK((f->m_Flags & FileFlags::Write) != 0, "Trying to write bytes to a file opened for reading");

	DWORD numBytesWritten = 0;
	if (::WriteFile(f->m_Handle, buffer, len, &numBytesWritten, NULL) == 0) {
		return 0;
	}

	return (uint32_t)numBytesWritten;
}

uint64_t fsFileGetSize(File* f)
{
	JX_CHECK(f != nullptr && f->m_Handle != INVALID_HANDLE_VALUE, "Trying to read from a null file");
	JX_CHECK((f->m_Flags & FileFlags::Read) != 0, "Cannot get the size of a file opened for writing");

	LARGE_INTEGER fileSize;
	if (::GetFileSizeEx(f->m_Handle, &fileSize) == 0) {
		JX_CHECK(false, "GetFileSizeEx() failed");
		return 0;
	}

	return (uint64_t)fileSize.QuadPart;
}

void fsFileSeek(File* f, int64_t offset, SeekOrigin::Enum origin)
{
	JX_CHECK(f != nullptr && f->m_Handle != INVALID_HANDLE_VALUE, "Trying to read from a null file");
	
	LONG offsetLow = (LONG)((offset & 0x00000000FFFFFFFF) >> 0);
	LONG offsetHigh = (LONG)((offset & 0xFFFFFFFF00000000) >> 32);
	DWORD moveMethod = origin == SeekOrigin::Begin ? FILE_BEGIN : (origin == SeekOrigin::Current ? FILE_CURRENT : FILE_END);
	DWORD result = ::SetFilePointer(f->m_Handle, offsetLow, &offsetHigh, moveMethod);
	JX_CHECK(result != INVALID_SET_FILE_POINTER, "SetFilePointer failed");
	BX_UNUSED(result); // for release mode
}

int64_t fsFileTell(File* f)
{
	JX_CHECK(f != nullptr && f->m_Handle != INVALID_HANDLE_VALUE, "Trying to read from a null file");

	LONG distanceHigh = 0;
	DWORD offsetLow = ::SetFilePointer(f->m_Handle, 0, &distanceHigh, FILE_CURRENT);
	JX_CHECK(offsetLow != INVALID_SET_FILE_POINTER, "SetFilePointer failed");

	return (int64_t)((uint64_t)offsetLow | ((uint64_t)distanceHigh << 32));
}

bool fsFileRemove(BaseDir::Enum baseDir, const char* relPath)
{
	if (baseDir == BaseDir::Install) {
		JX_CHECK(false, "Cannot remove files from installation folder");
		return false;
	}

	if (!setCurrentDirectory(baseDir)) {
		JX_CHECK(false, "Failed to set current working directory");
		return false;
	}

	wchar_t utf16RelPath[512];
	utf8ToUtf16(relPath, (uint16_t*)&utf16RelPath[0], BX_COUNTOF(utf16RelPath));

	::DeleteFileW(utf16RelPath);

	return true;
}

bool fsCreateFolderTree(BaseDir::Enum baseDir, const char* relPath)
{
	if (baseDir != BaseDir::UserData && baseDir != BaseDir::UserAppData) {
		JX_CHECK(false, "Can only create subfolders inside user or app data folder");
		return false;
	}

	if (!setCurrentDirectory(baseDir)) {
		JX_CHECK(false, "Failed to set current working directory");;
		return false;
	}

	wchar_t utf16RelPath[512];
	utf8ToUtf16(relPath, (uint16_t*)&utf16RelPath[0], BX_COUNTOF(utf16RelPath));

	wchar_t partialPath[512];

	const wchar_t* slash = wcschr(utf16RelPath, L'/');
	while (slash) {
		const uint32_t partialLen = (uint32_t)(slash - utf16RelPath);
		JX_CHECK(partialLen < BX_COUNTOF(partialPath) - 1, "Partial path too long!");

		const uint32_t copyLen = bx::uint32_min(partialLen, BX_COUNTOF(partialPath) - 1);
		bx::memCopy(partialPath, utf16RelPath, sizeof(wchar_t) * copyLen);
		partialPath[copyLen] = '\0';

		if (!createDirectory(partialPath)) {
			return false;
		}

		slash = wcschr(slash + 1, L'/');
	}

	return createDirectory(utf16RelPath);
}

bool fsRemoveEmptyFolder(BaseDir::Enum baseDir, const char* relPath)
{
	if (baseDir != BaseDir::UserData && baseDir != BaseDir::UserAppData) {
		JX_CHECK(false, "Can only create subfolders inside user data folder");
		return false;
	}

	if (!setCurrentDirectory(baseDir)) {
		JX_CHECK(false, "Failed to set current working directory");;
		return false;
	}

	wchar_t utf16RelPath[512];
	utf8ToUtf16(relPath, (uint16_t*)&utf16RelPath[0], BX_COUNTOF(utf16RelPath));

	::RemoveDirectoryW(utf16RelPath);

	return true;
}

bool fsEnumerateFiles(BaseDir::Enum baseDir, const char* relPath, EnumerateFilesCallback callback)
{
	const bool setcwdResult = setCurrentDirectory(baseDir);
	if (!setcwdResult) {
		JX_CHECK(false, "Failed to set current working directory");
		return false;
	}

	wchar_t utf16RelPath[512];
	utf8ToUtf16(relPath, (uint16_t*)&utf16RelPath[0], BX_COUNTOF(utf16RelPath));

	WIN32_FIND_DATAW ffd;
	HANDLE hFind = ::FindFirstFileW(utf16RelPath, &ffd);
	if (hFind == INVALID_HANDLE_VALUE) {
		return false;
	}

	char utf8Filename[1024];

	do {
		if (ffd.cFileName[0] == L'.') {
			if (ffd.cFileName[1] == L'\0' || (ffd.cFileName[1] == L'.' && ffd.cFileName[2] == L'\0')) {
				continue;
			}
		}

		utf8FromUtf16((uint16_t*)ffd.cFileName, &utf8Filename[0], BX_COUNTOF(utf8Filename));

		callback(utf8Filename, !(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
	} while (::FindNextFileW(hFind, &ffd) != 0);

	const DWORD dwError = ::GetLastError();

	::FindClose(hFind);

	return dwError == ERROR_NO_MORE_FILES;
}

const wchar_t* fsGetBaseDirPath(BaseDir::Enum baseDir)
{
	switch (baseDir) {
	case BaseDir::Install:      return s_FS->m_InstallFolder;
	case BaseDir::UserData:     return s_FS->m_UserDataFolder;
	case BaseDir::UserAppData:  return s_FS->m_UserAppDataFolder;
	case BaseDir::AbsolutePath: return L"";
	}

	return nullptr;
}

// TODO: Use CopyFile (https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-copyfile)
bool fsCopyFile(BaseDir::Enum srcBaseDir, const char* srcPath, BaseDir::Enum dstBaseDir, const char* dstPath)
{
	File* src = fsFileOpenRead(srcBaseDir, srcPath);
	if (!src) {
		return false;
	}

	File* dst = fsFileOpenWrite(dstBaseDir, dstPath);
	if (!dst) {
		fsFileClose(src);
		return false;
	}

	uint8_t buffer[1024];
	uint32_t numBytes = 0;
	while ((numBytes = fsFileReadBytes(src, buffer, 1024)) != 0) {
		fsFileWriteBytes(dst, buffer, numBytes);
	}

	fsFileClose(dst);
	fsFileClose(src);

	return true;
}

//////////////////////////////////////////////////////////////////////////
// Internal
//
static wchar_t* getInstallFolder()
{
	wchar_t folder[512];
	const uint32_t folderLen = ::GetModuleFileNameW(NULL, &folder[0], BX_COUNTOF(folder));
	if (folderLen == 0) {
		JX_CHECK(false, "GetModuleFileNameW() returned 0. Error: %08X", ::GetLastError());
		return nullptr;
	}

	wchar_t* lastSlash = wcsrchr(folder, L'\\');
	JX_CHECK(lastSlash != nullptr, "No forward slash in module filename");

	*(lastSlash + 1) = L'\0';

	const uint32_t len = (uint32_t)wcslen(folder);

	wchar_t* str = (wchar_t*)JX_ALLOC(sizeof(wchar_t) * (len + 1));
	if (!str) {
		return nullptr;
	}

	bx::memCopy(str, folder, sizeof(wchar_t) * len);
	str[len] = L'\0';
	return str;
}

static wchar_t* getUserDataFolder(const char* appName)
{
	wchar_t* folder = nullptr;
	if (FAILED(::SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &folder))) {
		::CoTaskMemFree(folder);
		return nullptr;
	}

	wchar_t utf16AppName[512];
	utf8ToUtf16(appName, (uint16_t*)&utf16AppName[0], BX_COUNTOF(utf16AppName));
	
	const uint32_t folderLen = (uint32_t)wcslen(folder);
	const uint32_t appNameLen = (uint32_t)wcslen(utf16AppName);
	const uint32_t len = folderLen + appNameLen + 2; // 2 * dir separator

	wchar_t* fullPath = (wchar_t*)JX_ALLOC(sizeof(wchar_t) * (len + 1));
	if (!fullPath) {
		::CoTaskMemFree(folder);
		return nullptr;
	}

	bx::memCopy(fullPath, folder, sizeof(wchar_t) * folderLen);
	fullPath[folderLen] = '\\';
	bx::memCopy(fullPath + folderLen + 1, utf16AppName, sizeof(wchar_t) * appNameLen);
	fullPath[folderLen + appNameLen + 1] = '\\';
	fullPath[folderLen + appNameLen + 2] = '\0';

	::CoTaskMemFree(folder);
	
	return fullPath;
}

static wchar_t* getAppDataFolder(const char* appName)
{
	wchar_t* folder = nullptr;
	if (FAILED(::SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &folder))) {
		::CoTaskMemFree(folder);
		return nullptr;
	}

	wchar_t utf16AppName[512];
	utf8ToUtf16(appName, (uint16_t*)&utf16AppName[0], BX_COUNTOF(utf16AppName));

	const uint32_t folderLen = (uint32_t)wcslen(folder);
	const uint32_t appNameLen = (uint32_t)wcslen(utf16AppName);
	const uint32_t len = folderLen + appNameLen + 2; // 2 * dir separator

	wchar_t* fullPath = (wchar_t*)JX_ALLOC(sizeof(wchar_t) * (len + 1));
	if (!fullPath) {
		::CoTaskMemFree(folder);
		return nullptr;
	}

	bx::memCopy(fullPath, folder, sizeof(wchar_t) * folderLen);
	fullPath[folderLen] = '\\';
	bx::memCopy(fullPath + folderLen + 1, utf16AppName, sizeof(wchar_t) * appNameLen);
	fullPath[folderLen + appNameLen + 1] = '\\';
	fullPath[folderLen + appNameLen + 2] = '\0';

	::CoTaskMemFree(folder);

	return fullPath;
}

static bool setCurrentDirectory(BaseDir::Enum baseDir)
{
	const wchar_t* dir = nullptr;
	switch (baseDir) {
	case BaseDir::Install:
		dir = s_FS->m_InstallFolder;
		break;
	case BaseDir::UserData:
		JX_CHECK(s_FS->m_UserDataFolder != nullptr, "User data folder hasn't been initialized.");
		dir = s_FS->m_UserDataFolder;
		break;
	case BaseDir::UserAppData:
		JX_CHECK(s_FS->m_UserAppDataFolder != nullptr, "App data folder hasn't been initialized.");
		dir = s_FS->m_UserAppDataFolder;
		break;
	case BaseDir::AbsolutePath:
		return true; // Nothing to do in this case.
	default:
		JX_CHECK(false, "Unknown base dir");
		return false;
	}

	return ::SetCurrentDirectoryW(dir) != 0;
}

static bool createDirectory(const wchar_t* path)
{
	if (!path) {
		return false;
	}
	
	bool success = true;

	const DWORD fileAttributes = ::GetFileAttributesW(path);
	if (fileAttributes == INVALID_FILE_ATTRIBUTES) {
		const BOOL result = ::CreateDirectoryW(path, nullptr);
		if (result == FALSE) {
			success = false;
		}
	} else {
		// Make sure this is a directory.
		const bool isDirectoryOrJunction = false
			|| ((fileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) 
			|| ((fileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0);
		if (!isDirectoryOrJunction) {
			success = false;
		}
	}

	return success;
}
}
#endif
