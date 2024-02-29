#pragma once
#include <ntddk.h>

#define LOG(format, ...) \
	logger::log(format, __VA_ARGS__)

#define LOG_ERROR() \
	logger::error()

namespace logger {
	template <typename ... T>
	__forceinline void log(const char* format, T const& ... args)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, format, args ...);
	}

	template <typename ... T>
	__forceinline void error()
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, 
			"[!] failed at %s:%d, ()\n", __FILE__, __LINE__);	// KeLowerIrql(irql);
	}
}
