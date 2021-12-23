#ifndef JX_SYS_H
#define JX_SYS_H

#include <stdint.h>
#include <bx/platform.h>

#ifndef JX_CONFIG_DEBUG
#	define JX_CONFIG_DEBUG 0
#endif

#ifndef JX_CONFIG_TRACE_ALLOCATIONS
#	define JX_CONFIG_TRACE_ALLOCATIONS 0
#endif

#ifndef JX_CONFIG_MATH_SIMD
#	define JX_CONFIG_MATH_SIMD 1
#endif

#ifndef JX_CONFIG_FRAME_ALLOCATOR_CAPACITY
#	define JX_CONFIG_FRAME_ALLOCATOR_CAPACITY (4 << 20)
#endif

#if JX_CONFIG_DEBUG
#include <bx/debug.h>

#define JX_TRACE(_format, ...) \
	do { \
		bx::debugPrintf(BX_FILE_LINE_LITERAL "jx " _format "\n", ##__VA_ARGS__); \
	} while(0)

#define JX_WARN(_condition, _format, ...) \
	do { \
		if (!(_condition) ) { \
			bx::debugPrintf(BX_FILE_LINE_LITERAL "jx " _format "\n", ##__VA_ARGS__); \
		} \
	} while(0)

#define JX_CHECK(_condition, _format, ...) \
	do { \
		if (!(_condition) ) { \
			bx::debugPrintf(BX_FILE_LINE_LITERAL "jx " _format "\n", ##__VA_ARGS__); \
			bx::debugBreak(); \
		} \
	} while(0)
#else
#define JX_TRACE(_format, ...)
#define JX_WARN(_condition, _format, ...)
#define JX_CHECK(_condition, _format, ...)
#endif

#define JX_NOT_IMPLEMENTED() JX_CHECK(false, "Not implemented yet");

namespace bx
{
struct AllocatorI;
}

namespace jx
{
struct Logger;

struct SystemInitFlags
{
	enum Enum : uint32_t
	{
		InitLog = 1u << 0
	};
};

bool initSystem(const char* appName, uint32_t sysFlags, uint32_t fsFlags);
void shutdownSystem();
void frame();

void setSystemLogger(Logger* logger);

bx::AllocatorI* createAllocator(const char* name);
void destroyAllocator(bx::AllocatorI* allocator);

bx::AllocatorI* getGlobalAllocator();
bx::AllocatorI* getFrameAllocator();
Logger* getGlobalLogger();

#if BX_PLATFORM_WINDOWS
void getLogFullPathW(Logger* logger, wchar_t* path, uint32_t maxLen);
#endif

void getCPUBrandString(char* str, uint32_t maxLen);
bool getOSFriendlyName(char* str, uint32_t maxLen);

#if JX_CONFIG_TRACE_ALLOCATIONS
uint32_t memTracerGetNumAllocators();
const char* memTracerGetAllocatorName(uint32_t id);
size_t memTracerGetTotalAllocatedMemory(uint32_t id);
#endif
}

#define JX_LOG_DEBUG(fmt, ...)   jx::logf(jx::getGlobalLogger(), jx::LogLevel::Debug, fmt, ##__VA_ARGS__)
#define JX_LOG_INFO(fmt, ...)    jx::logf(jx::getGlobalLogger(), jx::LogLevel::Info, fmt, ##__VA_ARGS__)
#define JX_LOG_WARN(fmt, ...)    jx::logf(jx::getGlobalLogger(), jx::LogLevel::Warning, fmt, ##__VA_ARGS__)
#define JX_LOG_ERROR(fmt, ...)   jx::logf(jx::getGlobalLogger(), jx::LogLevel::Error, fmt, ##__VA_ARGS__)

#endif
