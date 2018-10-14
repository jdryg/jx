#include <jx/fs.h>
#include <jx/allocator.h>
#include <jx/sys.h>
#include <jx/logger.h>
#include <bx/bx.h>
#include <bx/uint32_t.h>
#include <stdio.h>

#if BX_PLATFORM_EMSCRIPTEN
#include <unistd.h>
#include <dirent.h>
#include <string.h> // strchr
#include <sys/stat.h>
#include <errno.h>
#include <emscripten.h>

namespace jx
{
struct File
{
	FILE* m_Handle;
};

static bool s_FSRequiresSyncing = true;

static void invalidateFileSystem();

bool fsInit(const char* appName)
{
	BX_UNUSED(appName);

	EM_ASM(
		console.log("Creating and mounting IDBFS");
		FS.mkdir('/IDBFS');
		FS.mount(IDBFS, {}, '/IDBFS');

		Module.syncdone = 0;
		FS.syncfs(true, function(err) {
			if (err) {
				console.log("FS.syncfs(true) error:", err);
			} else {
				Module.print("FS.syncfs(true) finished (first time)");
				Module.syncdone = 1;
			}
		});
	);

	s_FSRequiresSyncing = false;

	return true;
}

void fsShutdown()
{
}

bool fsIsReady()
{
	return emscripten_run_script_int("Module.syncdone") == 1;
}

void fsSync()
{
	if (!s_FSRequiresSyncing) {
		return;
	}

	EM_ASM(
		console.log("Synchronizing file system");
		Module.syncdone = 0;
		FS.syncfs(false, function(err) {
			if (err) {
				console.log("FS.syncfs(false) error: ", err);
			} else {
				Module.print("FS.syncfs(false) finished");

				FS.syncfs(true, function(err) {
					if (err) {
						console.log("FS.syncfs(true) error: ", err);
					} else {
						Module.print("FS.syncfs(true) finished");
						Module.syncdone = 1;
					}
				});
			}
		});
	);

	s_FSRequiresSyncing = false;
}

File* fsFileOpenRead(BaseDir::Enum baseDir, const char* relPath)
{
	JX_LOG_DEBUG("fsFileOpenRead(%d, \"%s\")\n", baseDir, relPath);

	if (baseDir == BaseDir::Install) {
		chdir("/");
	} else if (baseDir == BaseDir::UserData) {
		chdir("/IDBFS");
	}

	FILE* handle = fopen(relPath, "rb");
	if (!handle) {
		JX_LOG_ERROR("Failed to open file \"%s\" for reading.\n", relPath);
		return nullptr;
	}

	File* f = (File*)JX_ALLOC(sizeof(File));
	if (!f) {
		JX_LOG_ERROR("Memory allocation failed (size: %u bytes)\n", sizeof(File));
		fclose(handle);
		return nullptr;
	}

	f->m_Handle = handle;

	return f;
}

File* fsFileOpenWrite(BaseDir::Enum baseDir, const char* relPath)
{
	JX_LOG_DEBUG("fsFileOpenWrite(%d, \"%s\")\n", baseDir, relPath);

	if (baseDir != BaseDir::UserData) {
		JX_CHECK(false, "Can only open files for writing in user data folder.");
		JX_LOG_ERROR("Tried to open read-only file \"%s\" for writing.\n", relPath);
		return nullptr;
	}

	chdir("/IDBFS");

	FILE* handle = handle = fopen(relPath, "wb");
	if (!handle) {
		JX_LOG_ERROR("Failed to open file \"%s\" for writing.\n", relPath);
		return nullptr;
	}

	File* f = (File*)JX_ALLOC(sizeof(File));
	if (!f) {
		JX_LOG_ERROR("Memory allocation failed (size: %u bytes)\n", sizeof(File));
		fclose(handle);
		return nullptr;
	}

	f->m_Handle = handle;

	return f;
}

void fsFileClose(File* f)
{
	JX_LOG_DEBUG("fsFileClose()\n");

	if (f->m_Handle) {
		fclose(f->m_Handle);
		f->m_Handle = nullptr;
	}
	JX_FREE(f);

	invalidateFileSystem();
}

uint32_t fsFileReadBytes(File* f, void* buffer, uint32_t len)
{
	return (uint32_t)fread(buffer, 1, len, f->m_Handle);
}

uint32_t fsFileWriteBytes(File* f, const void* buffer, uint32_t len)
{
	return (uint32_t)fwrite(buffer, 1, len, f->m_Handle);
}

uint64_t fsFileGetSize(File* f)
{
	int curPos = ftell(f->m_Handle);
	fseek(f->m_Handle, 0, SEEK_END);

	uint32_t size = (uint32_t)ftell(f->m_Handle);
	fseek(f->m_Handle, curPos, SEEK_SET);

	return (uint64_t)size;
}

void fsFileSeek(File* f, int offset, SeekOrigin::Enum origin)
{
	fseek(f->m_Handle, offset, origin);
}

bool fsFileRemove(BaseDir::Enum baseDir, const char* relPath)
{
	JX_LOG_DEBUG("fsFileRemove(%d, \"%s\")\n", baseDir, relPath);
	if (baseDir != BaseDir::UserData) {
		JX_CHECK(false, "Can only remove files from user data folder");
		JX_LOG_ERROR("Tried to remove read-only file \"%s\".\n", relPath);
		return false;
	}

	chdir("/IDBFS");
	if (remove(relPath) != 0) {
		JX_LOG_ERROR("Failed to remove file \"/IDBFS/%s\"\n", relPath);
		return false;
	}

	invalidateFileSystem();

	return true;
}

bool fsCreateFolderTree(BaseDir::Enum baseDir, const char* relPath)
{
	JX_LOG_DEBUG("fsCreateFolderTree(%d, \"%s\")\n", baseDir, relPath);

	if (baseDir != BaseDir::UserData) {
		JX_CHECK(false, "Can only create subfolders inside user data folder");
		JX_LOG_ERROR("Tried to create folder tree in read-only location.\n");
		return false;
	}

	chdir("/IDBFS");

	char partialPath[512];
	const char* slash = strchr(relPath, '/');
	while (slash) {
		const uint32_t partialLen = (uint32_t)(slash - relPath);
		JX_CHECK(partialLen < BX_COUNTOF(partialPath) - 1, "Partial path too long!");

		const uint32_t copyLen = bx::uint32_min(partialLen, BX_COUNTOF(partialPath) - 1);
		bx::memCopy(partialPath, relPath, sizeof(char) * copyLen);
		partialPath[copyLen] = '\0';

		const int res = mkdir(partialPath, S_IRWXU);
		JX_LOG_DEBUG("mkdir(\"%s\", S_IRWXU) = %d\n", partialPath, res);

		slash = strchr(slash + 1, '/');
	}

	const int res = mkdir(relPath, S_IRWXU);
	JX_LOG_DEBUG("mkdir(\"%s\", S_IRWXU) = %d\n", relPath, res);

	invalidateFileSystem();

	return true;
}

bool fsEnumerateFiles(BaseDir::Enum baseDir, const char* relPath, EnumerateFilesCallback callback)
{
	JX_LOG_DEBUG("fsEnumerateFiles(%d, \"%s\")\n", baseDir, relPath);

	if (baseDir == BaseDir::Install) {
		chdir("/");
	} else if (baseDir == BaseDir::UserData) {
		chdir("/IDBFS");
	}

	struct dirent* dir;

	// Remove the file pattern at the end of the pattern arg...
	const char* lastSlash = strrchr(relPath, '/');
	const size_t len = (size_t)(lastSlash - relPath);
	char folder[256];
	bx::memCopy(folder, relPath, len);
	folder[len] = '\0';

	DIR* d = opendir(folder);
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			JX_LOG_DEBUG("- %s\n", dir->d_name);
			if (dir->d_type == DT_REG) {
				callback(dir->d_name, true);
			}
		}

		closedir(d);
	} else {
		JX_LOG_ERROR("opendir(\"%s\") failed\n", folder);
	}

	return true;
}

static void invalidateFileSystem()
{
	s_FSRequiresSyncing = true;
}
} // namespace jx
#endif
