#ifndef JX_LOGGER_H
#define JX_LOGGER_H

#include <stdint.h>
#include <jtl/delegate.h>

namespace jx
{
struct Logger;

struct LogLevel
{
	enum Enum
	{
		Debug,
		Info,
		Warning,
		Error,
	};
};

struct LoggerFlags
{
	enum Enum : uint32_t
	{
		FlushOnEveryLog = 1 << 0,
		AppendTimestamp = 1 << 1,
		Multithreaded = 1 << 2,
	};
};

typedef jtl::delegate<void(LogLevel::Enum level, const char* str)> LoggingCallback;

Logger* createLog(const char* name, uint32_t flags);
void destroyLog(Logger* logger);

const char* loggerGetName(Logger* logger);
uint32_t loggerRegisterCallback(Logger* logger, LoggingCallback cb);
void loggerUnregisterCallback(Logger* logger, uint32_t cbID);

void logf(Logger* logger, LogLevel::Enum level, const char* fmt, ...);
}

#endif
