#include "vmx.hpp"
#include "stdafx.h"
#include "logger.hpp"

extern "C" {

	auto DriverUnload(_In_ PDRIVER_OBJECT driver_object) -> void {
		using hv::devirtualize_all_processors;

		UNICODE_STRING us_string{};
		RtlInitUnicodeString(PUNICODE_STRING(&us_string), DOS_NAME);
		IoDeleteSymbolicLink(PUNICODE_STRING(&us_string));
		
		if (driver_object->DeviceObject != nullptr)
			IoDeleteDevice(driver_object->DeviceObject);

		// Uninstall vmx on all processors
		//if (!vm_off) {
		//	devirtualize_all_processors();
		//	vm_off = true;
		//}

		return;
	}

	auto DefaultDispatch(_In_ PDEVICE_OBJECT, _In_ PIRP irp) -> NTSTATUS {
		//LOG("[*] Default Dispatch");

		irp->IoStatus.Status = STATUS_SUCCESS;
		irp->IoStatus.Information = 0;

		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	bool vm_off;
	unsigned g_num_processors;

	auto DriverEntry(_In_ PDRIVER_OBJECT driver_object, _In_ PUNICODE_STRING) -> NTSTATUS {
		using hv::virtualize_all_processors;
		using hv::launch_vm;
		using hv::launch_all_vmms;
		using vmx::vmx_is_vmx_available;

		vm_off = false;

		// Opt-in to using non-executable pool memory on Windows 8 and later.
		// https://msdn.microsoft.com/en-us/library/windows/hardware/hh920402(v=vs.85).aspx
		ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

		UNICODE_STRING us_drv_string{};
		RtlInitUnicodeString(PUNICODE_STRING(&us_drv_string), DRV_NAME);
		PDEVICE_OBJECT dev_obj{};

		auto ntstatus = IoCreateDevice(driver_object, 0, PUNICODE_STRING(&us_drv_string), FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, false, &dev_obj);
		if (!NT_SUCCESS(ntstatus)) {
			LOG("[!] Failure creating device object.\n");
			LOG_ERROR();
			return STATUS_FAILED_DRIVER_ENTRY;
		}

		UNICODE_STRING us_dos_string{};
		RtlInitUnicodeString(PUNICODE_STRING(&us_dos_string), DOS_NAME);
		ntstatus = IoCreateSymbolicLink(PUNICODE_STRING(&us_dos_string), PUNICODE_STRING(&us_drv_string));
		if (!NT_SUCCESS(ntstatus)) {
			LOG("[!] Failure creating dos devices.\n");
			LOG_ERROR();
			return STATUS_FAILED_DRIVER_ENTRY;
		}

		for (unsigned iter = 0; iter < IRP_MJ_MAXIMUM_FUNCTION; iter++)
			driver_object->MajorFunction[iter] = DefaultDispatch;

		driver_object->DriverUnload = DriverUnload;
		driver_object->Flags |= DO_BUFFERED_IO;

		g_num_processors = KeQueryActiveProcessorCount(NULL);

		if (!vmx_is_vmx_available())	return STATUS_FAILED_DRIVER_ENTRY;

		if (!virtualize_all_processors())	return STATUS_FAILED_DRIVER_ENTRY;

		launch_all_vmms();

		return STATUS_SUCCESS;
	}
}
