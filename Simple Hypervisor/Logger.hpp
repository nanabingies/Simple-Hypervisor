#pragma once
#include <ntddk.h>

#define LOG(format, ...) \
	logger::log(format, __VA_ARGS__)

#define LOG_EEROR(format, ...) \
	LOG("[!] failed at %s:%d, (0x%lX)\n", __FILE__, __LINE__, GetLastError())

namespace logger {
	template <typename ... T>
	__forceinline void log(const char* format, T const& ... args)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, format, args ...);
	}

	template <typename ... T>
	__forceinline void error(const char* format, T const& ... args)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, format, args ...);
	}
}
