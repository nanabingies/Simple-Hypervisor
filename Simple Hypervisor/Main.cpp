#include "vmx.hpp"
#include "stdafx.h"
#include "logger.hpp"

extern "C" {

	auto DriverUnload(_In_ PDRIVER_OBJECT) -> void {
		using hv::devirtualize_all_processors;

		// Uninstall vmx on all processors
		if (!vm_off) {
			devirtualize_all_processors();
			vm_off = true;
		}

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

	auto DriverEntry(_In_ PDRIVER_OBJECT, _In_ PUNICODE_STRING) -> NTSTATUS {
		using hv::virtualize_all_processors;
		using hv::launch_vm;
		using hv::launch_all_vmms;
		using vmx::vmx_is_vmx_available;

		vm_off = false;

		// Opt-in to using non-executable pool memory on Windows 8 and later.
		// https://msdn.microsoft.com/en-us/library/windows/hardware/hh920402(v=vs.85).aspx
		ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

		g_num_processors = KeQueryActiveProcessorCount(NULL);

		if (!vmx_is_vmx_available())	return STATUS_FAILED_DRIVER_ENTRY;

		if (!virtualize_all_processors())	return STATUS_FAILED_DRIVER_ENTRY;

		launch_all_vmms();

		return STATUS_SUCCESS;
	}
}
