#ifndef JX_LOGGER_H
#define JX_LOGGER_H

#include <stdint.h>

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

Logger* createLog(const char* name, uint32_t flags);
void destroyLog(Logger* logger);

void logf(Logger* logger, LogLevel::Enum level, const char* fmt, ...);
}

#endif
