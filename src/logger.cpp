#include <jx/logger.h>
#include <jx/fs.h>
#include <jx/allocator.h>
#include <jx/str.h>
#include <bx/string.h>
#include <bx/mutex.h>
#include <time.h>

#if BX_PLATFORM_WINDOWS
#include <Windows.h>
#endif

#if BX_PLATFORM_EMSCRIPTEN
#include <stdio.h>
#endif

namespace jx
{
struct CallbackData
{
	LoggingCallback m_Func;
	void* m_UserData;
	bool m_Active;
};

struct Logger
{
	char* m_Name;
	File* m_File;
#if BX_CONFIG_SUPPORTS_THREADING
	bx::Mutex* m_Mutex;
#endif
	CallbackData* m_Callbacks;
	uint32_t m_NumCallbacks;
	uint32_t m_Flags;
};

Logger* createLog(jx::BaseDir::Enum baseDir, const char* name, uint32_t flags)
{
	Logger* logger = (Logger*)JX_ALLOC(sizeof(Logger));
	if (!logger) {
		JX_CHECK(false, "Failed to allocator Logger");
		return nullptr;
	}
	
	bx::memSet(logger, 0, sizeof(Logger));

	if (name != nullptr) {
		logger->m_Name = jx::strDup(name);

#if BX_PLATFORM_WINDOWS || BX_PLATFORM_LINUX || BX_PLATFORM_OSX || BX_PLATFORM_RPI
		char logFilename[256];
		bx::snprintf(logFilename, 256, "%s.log", name);
		logger->m_File = fsFileOpenWrite(baseDir, logFilename);
		if (!logger->m_File) {
			JX_CHECK(false, "Failed to open log file");
			JX_FREE(logger);
			return nullptr;
		}
#else
		BX_UNUSED(name);
		logger->m_File = nullptr;
#endif
	}

	logger->m_Flags = flags;

#if BX_CONFIG_SUPPORTS_THREADING
	if ((flags & LoggerFlags::Multithreaded) != 0) {
		logger->m_Mutex = JX_NEW(bx::Mutex)();
	} else {
		logger->m_Mutex = nullptr;
	}
#endif

	return logger;
}

void destroyLog(Logger* logger)
{
#if BX_PLATFORM_WINDOWS || BX_PLATFORM_LINUX || BX_PLATFORM_OSX || BX_PLATFORM_RPI
	if (logger->m_File) {
		fsFileClose(logger->m_File);
		logger->m_File = nullptr;
	}
#endif

#if BX_CONFIG_SUPPORTS_THREADING
	if (logger->m_Mutex) {
		JX_DELETE(logger->m_Mutex);
		logger->m_Mutex = nullptr;
	}
#endif

	jx::strFree(logger->m_Name);

	JX_FREE(logger);
}

const char* loggerGetName(Logger* logger)
{
	return logger->m_Name;
}

uint32_t loggerRegisterCallback(Logger* logger, LoggingCallback cb, void* userData)
{
	const uint32_t oldCapacity = logger->m_NumCallbacks;
	const uint32_t newCapacity = oldCapacity + 1;
	CallbackData* newCallbacks = (CallbackData*)JX_ALLOC(sizeof(CallbackData) * newCapacity);
	if (!newCallbacks) {
		return UINT32_MAX;
	}

	bx::memCopy(newCallbacks, logger->m_Callbacks, sizeof(CallbackData) * oldCapacity);
	bx::memSet(&newCallbacks[oldCapacity], 0, sizeof(CallbackData));
	
	JX_FREE(logger->m_Callbacks);
	logger->m_Callbacks = newCallbacks;
	logger->m_NumCallbacks = newCapacity;

	const uint32_t cbID = oldCapacity;
	CallbackData* cbData = &logger->m_Callbacks[cbID];
	cbData->m_Func = cb;
	cbData->m_UserData = userData;
	cbData->m_Active = true;

	return cbID;
}

void loggerUnregisterCallback(Logger* logger, uint32_t cbID)
{
	logger->m_Callbacks[cbID].m_Active = false;
}

void logf(Logger* logger, LogLevel::Enum level, const char* fmt, ...)
{
	// NOTE: This can happen when using JX_LOG_xxx macros and the system has been initialized
	// without jx::SystemInitFlags::InitLog.
	if (!logger) {
		return;
	}

#if BX_PLATFORM_WINDOWS || BX_PLATFORM_LINUX || BX_PLATFORM_OSX || BX_PLATFORM_RPI
//	JX_CHECK(logger->m_File != nullptr, "Logger doesn't have a valid file handle");
#endif

	static char logLine[2048] = { 0 };

	const bool forceFlush = (logger->m_Flags & LoggerFlags::FlushOnEveryLog) != 0;
	const bool appendTimestamp = (logger->m_Flags & LoggerFlags::AppendTimestamp) != 0;

#if BX_CONFIG_SUPPORTS_THREADING
	const bool multithreaded = (logger->m_Flags & LoggerFlags::Multithreaded) != 0;

	if (multithreaded) {
		logger->m_Mutex->lock();
	}
#endif

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

#if BX_PLATFORM_WINDOWS || BX_PLATFORM_LINUX || BX_PLATFORM_OSX || BX_PLATFORM_RPI
	if (logger->m_File) {
		if (levelSymbol) {
			fsFileWriteBytes(logger->m_File, levelSymbol, 4);
		}

		if (appendTimestamp) {
			char timestamp[128];
#if !BX_PLATFORM_WINDOWS
			time_t rawtime;
			time(&rawtime);
			struct tm* timeinfo = localtime(&rawtime);
			strftime(timestamp, BX_COUNTOF(timestamp), "%Y-%m-%d %H:%M:%S ", timeinfo);
#else
			SYSTEMTIME sysTime;
#if 0
			::GetSystemTime(&sysTime);
#else
			::GetLocalTime(&sysTime);
#endif
			bx::snprintf(timestamp, BX_COUNTOF(timestamp), "%04u-%02u-%02u %02u:%02u:%02u.%03u ", sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds);
#endif

			fsFileWriteBytes(logger->m_File, timestamp, bx::strLen(timestamp));
		}

		fsFileWriteBytes(logger->m_File, logLine, bx::min<int>(logLineLen, BX_COUNTOF(logLine) - 1));

		if (forceFlush) {
			// TODO: fflush?
		}
	}
#else
	BX_UNUSED(logLineLen);
	BX_UNUSED(forceFlush);

	if (levelSymbol) {
		printf("%s", levelSymbol);
	}

	if (appendTimestamp) {
		char timestamp[128];
		time_t rawtime;
		time(&rawtime);
		struct tm* timeinfo = localtime(&rawtime);
		strftime(timestamp, BX_COUNTOF(timestamp), "%Y-%m-%d %H:%M:%S ", timeinfo);

		printf("%s", timestamp);
	}

	printf("%s", logLine);
#endif

	const uint32_t numCallbacks = logger->m_NumCallbacks;
	for (uint32_t i = 0; i < numCallbacks; ++i) {
		CallbackData* cb = &logger->m_Callbacks[i];
		if (cb->m_Active) {
			cb->m_Func(level, logLine, cb->m_UserData);
		}
	}

#if BX_CONFIG_SUPPORTS_THREADING
	if (multithreaded) {
		logger->m_Mutex->unlock();
	}
#endif
}
}
