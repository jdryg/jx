#include <jx/sys.h>
#include <jx/rand.h>
#include <jx/fs.h>
#include <jx/logger.h>
#include <jx/linear_allocator.h>
#include <bx/allocator.h>
#include <chrono>

#if BX_CPU_X86
#if BX_PLATFORM_WINDOWS
#include <intrin.h> // __cpuid()
#include <Windows.h>
#else
#include <cpuid.h> // __get_cpuid()
#endif
#endif

#if JX_CONFIG_TRACE_ALLOCATIONS
#include "memory_tracer.h"
#include "tracing_allocator.h"
#endif

namespace jx
{
struct Context
{
	bx::AllocatorI* m_SystemAllocator;
	bx::AllocatorI* m_GlobalAllocator;
	LinearAllocator* m_FrameAllocator;
	Logger* m_Logger;
};

static Context* s_Context = nullptr;

static bx::AllocatorI* getSystemAllocator();

bool initSystem(const char* appName)
{
	JX_CHECK(s_Context == nullptr, "System already initialized");

	bx::AllocatorI* systemAllocator = getSystemAllocator();

#if JX_CONFIG_TRACE_ALLOCATIONS
	memTracerInit(systemAllocator);
#endif

	const uint32_t totalMem = sizeof(Context);

	uint8_t* mem = (uint8_t*)BX_ALLOC(systemAllocator, totalMem);
	if (!mem) {
		return false;
	}

	// Initialize the global context
	s_Context = (Context*)mem;
	s_Context->m_SystemAllocator = systemAllocator;
	s_Context->m_GlobalAllocator = createAllocator("Global");

	// Initialize temporary/frame allocator
	s_Context->m_FrameAllocator = BX_NEW(systemAllocator, LinearAllocator)(systemAllocator, JX_CONFIG_FRAME_ALLOCATOR_CAPACITY);
	
	// Initialize the filesystem
	if (!fsInit(appName)) {
		JX_CHECK(false, "Failed to initialize file system");
		return false;
	}

	s_Context->m_Logger = createLog(appName, LoggerFlags::AppendTimestamp | LoggerFlags::FlushOnEveryLog | LoggerFlags::Multithreaded);
	if (!s_Context->m_Logger) {
		JX_CHECK(false, "Failed to initialize logger");
		return false;
	}

	JX_LOG_INFO("System initialized\n");

	// Initialize the PRNG...
	auto now = std::chrono::high_resolution_clock::now();
	auto duration = now.time_since_epoch();
	uint64_t millis = (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
	jx::seed((uint32_t)((millis >> 32) & 0x00000000FFFFFFFF) ^ (uint32_t)(millis & 0x00000000FFFFFFFF));

	return true;
}

void shutdownSystem()
{
	JX_CHECK(s_Context != nullptr, "System hasn't been initialized yet.");

	bx::AllocatorI* systemAllocator = getSystemAllocator();

	fsShutdown();

	destroyLog(s_Context->m_Logger);
	s_Context->m_Logger = nullptr;

	BX_DELETE(systemAllocator, s_Context->m_FrameAllocator);
	destroyAllocator(s_Context->m_GlobalAllocator);

#if JX_CONFIG_TRACE_ALLOCATIONS
	memTracerShutdown();
#endif

	BX_FREE(systemAllocator, s_Context);
	s_Context = nullptr;
}

void frame()
{
	s_Context->m_FrameAllocator->freeAll();
}

bx::AllocatorI* createAllocator(const char* name)
{
	Context* ctx = s_Context;

#if JX_CONFIG_TRACE_ALLOCATIONS
	return BX_NEW(ctx->m_SystemAllocator, TracingAllocator)(name, ctx->m_SystemAllocator);
#else
	BX_UNUSED(name);
	return BX_NEW(ctx->m_SystemAllocator, bx::DefaultAllocator)();
#endif
}

void destroyAllocator(bx::AllocatorI* allocator)
{
	Context* ctx = s_Context;

	BX_DELETE(ctx->m_SystemAllocator, allocator);
}

bx::AllocatorI* getGlobalAllocator()
{
	return s_Context->m_GlobalAllocator;
}

bx::AllocatorI* getFrameAllocator()
{
	return s_Context->m_FrameAllocator;
}

Logger* getGlobalLogger()
{
	return s_Context->m_Logger;
}

void getCPUBrandString(char* str, uint32_t maxLen)
{
#if BX_CPU_X86
	JX_CHECK(maxLen > 48, "Short string");
	BX_UNUSED(maxLen);

#if BX_PLATFORM_WINDOWS
	// Calling __cpuid with 0x80000000 as the InfoType argument
	// gets the number of valid extended IDs.
	int cpuInfo[4];
	__cpuid(cpuInfo, 0x80000000);
	const uint32_t nExIds = (uint32_t)cpuInfo[0];

	// Get the information associated with each extended ID.
	str[0] = '\0';
	for (uint32_t i = 0x80000002; i <= nExIds; ++i) {
		__cpuid(cpuInfo, i);

		// Interpret CPU brand string.
		if (i == 0x80000002) {
			bx::memCopy(str, cpuInfo, 16);
			str[16] = '\0';
		} else if (i == 0x80000003) {
			bx::memCopy(str + 16, cpuInfo, 16);
			str[32] = '\0';
		} else if (i == 0x80000004) {
			bx::memCopy(str + 32, cpuInfo, 16);
			str[48] = '\0';
			break;
		}
	}
#else // BX_PLATFORM_WINDOWS
	// Calling __cpuid with 0x80000000 as the InfoType argument
	// gets the number of valid extended IDs.
	uint32_t cpuInfo[4];
	__get_cpuid(0x80000000, &cpuInfo[0], &cpuInfo[1], &cpuInfo[2], &cpuInfo[3]);

	uint32_t nExIds = cpuInfo[0];

	// Get the information associated with each extended ID.
	str[0] = '\0';
	for (uint32_t i = 0x80000002; i <= nExIds; ++i) {
		__get_cpuid(i, &cpuInfo[0], &cpuInfo[1], &cpuInfo[2], &cpuInfo[3]);

		// Interpret CPU brand string.
		if (i == 0x80000002) {
			bx::memCopy(str, cpuInfo, 16);
			str[16] = '\0';
		} else if (i == 0x80000003) {
			bx::memCopy(str + 16, cpuInfo, 16);
			str[32] = '\0';
		} else if (i == 0x80000004) {
			bx::memCopy(str + 32, cpuInfo, 16);
			str[48] = '\0';
			break;
		}
	}
#endif // BX_PLATFORM_WINDOWS
#else // BX_CPU_X86
#if BX_PLATFORM_RPI
	str[0] = 'R';
	str[1] = 'P';
	str[2] = 'i';
	str[3] = 0;
#elif BX_PLATFORM_EMSCRIPTEN
	bx::snprintf(str, maxLen, "Emscripten");
#endif // BX_PLATFORM_RPI
#endif // BX_CPU_X86
}

bool getOSFriendlyName(char* str, uint32_t maxLen)
{
#if BX_PLATFORM_WINDOWS
	str[0] = '\0';

	HKEY curVersionKey;
	LONG lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &curVersionKey);
	if (lResult != ERROR_SUCCESS) {
		return false;
	}

	uint8_t buffer[1024];
	ULONG bufferSize = 1023;
	if (RegQueryValueEx(curVersionKey, "ProductName", nullptr, nullptr, buffer, &bufferSize) == ERROR_SUCCESS) {
		strcat_s(str, maxLen, (char*)buffer);
	}

	bufferSize = 1023;
	if (RegQueryValueEx(curVersionKey, "CurrentBuildNumber", nullptr, nullptr, buffer, &bufferSize) == ERROR_SUCCESS) {
		strcat_s(str, maxLen, " Build ");
		strcat_s(str, maxLen, (char*)buffer);
	}

	bufferSize = 1023;
	if (RegQueryValueEx(curVersionKey, "CSDVersion", nullptr, nullptr, buffer, &bufferSize) == ERROR_SUCCESS) {
		strcat_s(str, maxLen, " ");
		strcat_s(str, maxLen, (char*)buffer);
	}

	RegCloseKey(curVersionKey);
#elif BX_PLATFORM_LINUX
	bx::snprintf(str, maxLen, "Linux x64");
#elif BX_PLATFORM_RPI
	bx::snprintf(str, maxLen, "RaspberryPi");
#elif BX_PLATFORM_OSX
	bx::snprintf(str, maxLen, "Mac OS X x64");
#elif BX_PLATFORM_EMSCRIPTEN
	bx::snprintf(str, maxLen, "Emscripten");
#else
	return false;
#endif

	return true;
}

#if BX_PLATFORM_WINDOWS
void getLogFullPathW(Logger* logger, wchar_t* path, uint32_t maxLen)
{
	const char* loggerName = loggerGetName(logger);
	wchar_t nameW[256];
	mbstowcs(nameW, loggerName, BX_COUNTOF(nameW));

	swprintf(path, maxLen, L"%s%s.log", fsGetBaseDirPath(BaseDir::UserData), nameW);
}
#endif

// Internal
static bx::AllocatorI* getSystemAllocator()
{
	static BX_ALIGN_DECL(16, char) _AllocatorBuffer[sizeof(bx::DefaultAllocator)];
	static bx::AllocatorI* systemAllocator = BX_PLACEMENT_NEW(_AllocatorBuffer, bx::DefaultAllocator)();

	return systemAllocator;
}
}
