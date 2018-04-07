#include <jx/logger.h>
#include <jx/fs.h>
#include <jx/allocator.h>
#include <bx/string.h>
#include <bx/mutex.h>
#include <time.h>

namespace jx
{
struct Logger
{
	File* m_File;
	uint32_t m_Flags;
	bx::Mutex* m_Mutex;
};

Logger* createLog(const char* name, uint32_t flags)
{
	Logger* logger = (Logger*)JX_ALLOC(sizeof(Logger));
	if (!logger) {
		JX_CHECK(false, "Failed to allocator Logger");
		return nullptr;
	}
	
	bx::memSet(logger, 0, sizeof(Logger));

	char logFilename[256];
	bx::snprintf(logFilename, 256, "%s.log", name);
	logger->m_File = fsFileOpenWrite(BaseDir::UserData, logFilename);
	if (!logger->m_File) {
		JX_CHECK(false, "Failed to open log file");
		JX_FREE(logger);
		return nullptr;
	}

	logger->m_Flags = flags;

	if ((flags & LoggerFlags::Multithreaded) != 0) {
		logger->m_Mutex = JX_NEW(bx::Mutex)();
	} else {
		logger->m_Mutex = nullptr;
	}

	return logger;
}

void destroyLog(Logger* logger)
{
	if (logger->m_File) {
		fsFileClose(logger->m_File);
		logger->m_File = nullptr;
	}

	if (logger->m_Mutex) {
		JX_DELETE(logger->m_Mutex);
		logger->m_Mutex = nullptr;
	}

	JX_FREE(logger);
}

void logf(Logger* logger, LogLevel::Enum level, const char* fmt, ...)
{
	JX_CHECK(logger != nullptr, "Null logger passed");
	JX_CHECK(logger->m_File != nullptr, "Logger doesn't have a valid file handle");

	static char logLine[2048];

	const bool forceFlush = (logger->m_Flags & LoggerFlags::FlushOnEveryLog) != 0;
	const bool appendTimestamp = (logger->m_Flags & LoggerFlags::AppendTimestamp) != 0;
	const bool multithreaded = (logger->m_Flags & LoggerFlags::Multithreaded) != 0;

	if (multithreaded) {
		logger->m_Mutex->lock();
	}

	va_list ap;
	va_start(ap, fmt);
	const int logLineLen = bx::vsnprintf(logLine, BX_COUNTOF(logLine), fmt, ap);
	va_end(ap);

	const char* levelSymbol = nullptr;
	switch (level) {
	case LogLevel::Debug:   levelSymbol = "(d) "; break;
	case LogLevel::Info:    levelSymbol = "(i) "; break;
	case LogLevel::Warning: levelSymbol = "(!) "; break;
	case LogLevel::Error:   levelSymbol = "(x) "; break;
	default: break;
	}

	if (levelSymbol) {
		fsFileWriteBytes(logger->m_File, levelSymbol, 4);
	}

	if (appendTimestamp) {
		char timestamp[128];
		time_t rawtime;
		time(&rawtime);
		struct tm* timeinfo = localtime(&rawtime);
		strftime(timestamp, BX_COUNTOF(timestamp), "%Y-%m-%d %H:%M:%S ", timeinfo);

		fsFileWriteBytes(logger->m_File, timestamp, bx::strLen(timestamp));
	}

	fsFileWriteBytes(logger->m_File, logLine, bx::min<int>(logLineLen, BX_COUNTOF(logLine) - 1));

	if (forceFlush) {
		// TODO: fflush?
	}

	if (multithreaded) {
		logger->m_Mutex->unlock();
	}
}
}
