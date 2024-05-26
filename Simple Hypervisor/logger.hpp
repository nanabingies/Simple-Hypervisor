#pragma once
#include <ntddk.h>

#define LOG(format, ...) \
	logger::log(format, __VA_ARGS__)

#define LOG_ERROR(file, line) \
	logger::error(file, line)

namespace logger {
	template <typename ... T>
	__forceinline void log(const char* format, T const& ... args)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, format, args ...);	// DPFLTR_INFO_LEVEL
	}

	template <typename ... T>
	__forceinline void error(char* file, int line)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, 
			"[!] failed at %s:%d, ()\n", file, line);	// KeLowerIrql(irql);
	}
}
