#include <jx/fs.h>
#include <jx/sys.h>
#include <jx/logger.h>
#include <jx/allocator.h>
#include <bx/bx.h>
#include <stdio.h>

#if BX_PLATFORM_EMSCRIPTEN
#include <unistd.h>
#include <dirent.h>
#include <emscripten.h>

namespace jx
{
struct File
{
	FILE* m_Handle;
};

bool fsInit(const char* appName)
{
	BX_UNUSED(appName);

	JX_LOG_DEBUG("fsInit()");
	EM_ASM(
		console.log("Creating and mounting IDBFS");
		FS.mkdir('/IDBFS');
		FS.mount(IDBFS, {}, '/IDBFS');
		
		Module.syncdone = 0;
		FS.syncfs(true, function (err) {
			if(err) {
				console.log(err);
			} else {
				Module.print("FS.syncfs(true) finished (first time)");
				Module.syncdone = 1;
			}
		});
	);

	return true;
}

void fsShutdown()
{
}

bool fsIsReady()
{
	return emscripten_run_script_int("Module.syncdone") == 1;
}

File* fsFileOpenRead(BaseDir::Enum baseDir, const char* relPath)
{
	JX_LOG_DEBUG("fsFileOpenRead(%d, \"%s\")", baseDir, relPath);
	
	File* f = nullptr;
	if (baseDir == BaseDir::Install) {
		f = (File*)JX_ALLOC(sizeof(File));
		f->m_Handle = fopen(relPath, "rb");
	} 

	return f;
}

File* fsFileOpenWrite(BaseDir::Enum baseDir, const char* relPath)
{
	JX_LOG_DEBUG("fsFileOpenWrite(%d, \"%s\")", baseDir, relPath);

	return nullptr;
}

void fsFileClose(File* f)
{
	JX_LOG_DEBUG("fsFileClose()");
	if (f->m_Handle) {
		fclose(f->m_Handle);
		f->m_Handle = nullptr;
	}
	JX_FREE(f);
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
	JX_LOG_DEBUG("fsFileRemove(%d, \"%s\")", baseDir, relPath);

	return false;
}

bool fsCreateFolderTree(BaseDir::Enum baseDir, const char* relPath)
{
	return false;
}

bool fsEnumerateFiles(BaseDir::Enum baseDir, const char* relPath, EnumerateFilesCallback callback)
{
	return false;
}
}
#endif
