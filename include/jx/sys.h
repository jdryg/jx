#ifndef JX_SYS_H
#define JX_SYS_H

#include <stdint.h>

#ifndef JX_CONFIG_DEBUG
#	define JX_CONFIG_DEBUG _DEBUG
#endif

#ifndef JX_CONFIG_TRACE_ALLOCATIONS
#	define JX_CONFIG_TRACE_ALLOCATIONS 0
#endif

#ifndef JX_CONFIG_MATH_SIMD
#	define JX_CONFIG_MATH_SIMD 1
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
			JX_TRACE(BX_FILE_LINE_LITERAL _format, ##__VA_ARGS__); \
		} \
	} while(0)

#define JX_CHECK(_condition, _format, ...) \
	do { \
		if (!(_condition) ) { \
			JX_TRACE(BX_FILE_LINE_LITERAL _format, ##__VA_ARGS__); \
			bx::debugBreak(); \
		} \
	} while(0)
#else
#define JX_TRACE(_format, ...)
#define JX_WARN(_condition, _format, ...)
#define JX_CHECK(_condition, _format, ...)
#endif

namespace bx
{
struct AllocatorI;
}

namespace jx
{
struct Logger;

bool initSystem(const char* appName);
void shutdownSystem();

bx::AllocatorI* createAllocator(const char* name);
void destroyAllocator(bx::AllocatorI* allocator);

bx::AllocatorI* getGlobalAllocator();
Logger* getGlobalLogger();

void getCPUBrandString(char* str, uint32_t maxLen);
bool getOSFriendlyName(char* str, uint32_t maxLen);
}

#define JX_LOG_DEBUG(fmt, ...)   jx::logf(jx::getGlobalLogger(), jx::LogLevel::Debug, fmt, ##__VA_ARGS__)
#define JX_LOG_INFO(fmt, ...)    jx::logf(jx::getGlobalLogger(), jx::LogLevel::Info, fmt, ##__VA_ARGS__)
#define JX_LOG_WARN(fmt, ...)    jx::logf(jx::getGlobalLogger(), jx::LogLevel::Warning, fmt, ##__VA_ARGS__)
#define JX_LOG_ERROR(fmt, ...)   jx::logf(jx::getGlobalLogger(), jx::LogLevel::Error, fmt, ##__VA_ARGS__)

#endif
