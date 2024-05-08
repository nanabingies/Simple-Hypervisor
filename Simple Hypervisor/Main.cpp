#include "vmx.hpp"
#include "stdafx.h"
#include "logger.hpp"

extern "C" {

	auto DriverUnload(_In_ PDRIVER_OBJECT driver_object) -> void {
		using vmx::disable_vmx;
		using vmx::vmx_free_vmm_context;

		disable_vmx();
		vmx_free_vmm_context();

		UNICODE_STRING us_string{};
		RtlInitUnicodeString(PUNICODE_STRING(&us_string), DOS_NAME);
		IoDeleteSymbolicLink(PUNICODE_STRING(&us_string));
		
		if (driver_object->DeviceObject != nullptr)
			IoDeleteDevice(driver_object->DeviceObject);

		// Uninstall vmx on all processors

		return;
	}

	auto DefaultDispatch(_In_ PDEVICE_OBJECT, _In_ PIRP irp) -> NTSTATUS {
		//LOG("[*] Default Dispatch");

		irp->IoStatus.Status = STATUS_SUCCESS;
		irp->IoStatus.Information = 0;

		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	__vmm_context* g_vmm_context = nullptr;

	auto DriverEntry(_In_ PDRIVER_OBJECT driver_object, _In_ PUNICODE_STRING) -> NTSTATUS {
		using hv::vmm_init;
		using vmx::disable_vmx;
		using vmx::vmx_is_vmx_available;
		using vmx::vmx_free_vmm_context;

		// Opt-in to using non-executable pool memory on Windows 8 and later.
		// https://msdn.microsoft.com/en-us/library/windows/hardware/hh920402(v=vs.85).aspx
		ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

		UNICODE_STRING us_drv_string{};
		RtlInitUnicodeString(PUNICODE_STRING(&us_drv_string), DRV_NAME);
		PDEVICE_OBJECT dev_obj{};

		auto ntstatus = IoCreateDevice(driver_object, 0, PUNICODE_STRING(&us_drv_string), FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, false, &dev_obj);
		if (!NT_SUCCESS(ntstatus)) {
			LOG("[!] Failure creating device object.\n");
			return STATUS_FAILED_DRIVER_ENTRY;
		}

		UNICODE_STRING us_dos_string{};
		RtlInitUnicodeString(PUNICODE_STRING(&us_dos_string), DOS_NAME);
		ntstatus = IoCreateSymbolicLink(PUNICODE_STRING(&us_dos_string), PUNICODE_STRING(&us_drv_string));
		if (!NT_SUCCESS(ntstatus)) {
			LOG("[!] Failure creating dos devices.\n");
			return STATUS_FAILED_DRIVER_ENTRY;
		}

		for (unsigned iter = 0; iter < IRP_MJ_MAXIMUM_FUNCTION; iter++)
			driver_object->MajorFunction[iter] = DefaultDispatch;

		driver_object->DriverUnload = DriverUnload;
		driver_object->Flags |= DO_BUFFERED_IO;

		if (!vmx_is_vmx_available())	return STATUS_FAILED_DRIVER_ENTRY;

		if (!vmm_init()) {
			disable_vmx();
			vmx_free_vmm_context();
			return STATUS_FAILED_DRIVER_ENTRY;
		}

		return STATUS_SUCCESS;
	}
}
