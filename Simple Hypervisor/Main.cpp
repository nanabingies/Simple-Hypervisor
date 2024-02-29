#include "stdafx.h"

extern "C" {

	auto DriverEntry(_In_ PDRIVER_OBJECT driver_object, _In_ PUNICODE_STRING registry_path) -> NTSTATUS {
		DbgPrint("[*] Loading file %wz\n", registry_path);

		// Opt-in to using non-executable pool memory on Windows 8 and later.
		// https://msdn.microsoft.com/en-us/library/windows/hardware/hh920402(v=vs.85).aspx
		ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

		NTSTATUS status;
		UNICODE_STRING drv_name;
		PDEVICE_OBJECT device_object;

		RtlInitUnicodeString(&drv_name, DRV_NAME);

		status = IoCreateDevice(driver_object, 0, &drv_name, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN,
			FALSE, reinterpret_cast<PDEVICE_OBJECT*>(&device_object));
		if (!NT_SUCCESS(status))	return STATUS_FAILED_DRIVER_ENTRY;

		DbgPrint("[*] Successfully created device object\n");

		DbgPrint("[*] The hypervisor has been installed.\n");

		return STATUS_SUCCESS;
	}
}