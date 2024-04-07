#include "vmx.hpp"
#include "stdafx.h"
#include "logger.hpp"

extern "C" {

	auto DriverUnload(_In_ PDRIVER_OBJECT driver_object) -> void {
		using hv::devirtualize_all_processors;

		UNREFERENCED_PARAMETER(driver_object);

		//LOG("[*] Terminating VMs on processors...");

		// Uninstall vmx on all processors
		devirtualize_all_processors();

		//if (driver_object->DeviceObject != nullptr) {
		//	IoDeleteDevice(driver_object->DeviceObject);
		//}

		//LOG("[*] Unloading Device Driver...");
		return;
	}

	auto DefaultDispatch(_In_ PDEVICE_OBJECT, _In_ PIRP irp) -> NTSTATUS {
		//LOG("[*] Default Dispatch");

		irp->IoStatus.Status = STATUS_SUCCESS;
		irp->IoStatus.Information = 0;

		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	unsigned g_num_processors;

	auto DriverEntry(_In_ PDRIVER_OBJECT, _In_ PUNICODE_STRING) -> NTSTATUS {
		using hv::virtualize_all_processors;
		using hv::init_vmcs;
		using hv::launch_vm;
		using hv::launch_all_vmms;
		using vmx::create_vcpus;
		using vmx::vmx_is_vmx_available;

		// Opt-in to using non-executable pool memory on Windows 8 and later.
		// https://msdn.microsoft.com/en-us/library/windows/hardware/hh920402(v=vs.85).aspx
		ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

		if (!vmx_is_vmx_available())	return STATUS_FAILED_DRIVER_ENTRY;
		if (!create_vcpus())	return STATUS_FAILED_DRIVER_ENTRY;
		LOG("Done vmx initializations.\n");

		//if (!virtualize_all_processors())	return STATUS_FAILED_DRIVER_ENTRY;

		//KeIpiGenericCall(static_cast<PKIPI_BROADCAST_WORKER>(launch_vm), 0);
		//launch_all_vmms();
		KeIpiGenericCall(static_cast<PKIPI_BROADCAST_WORKER>(init_vmcs), 0);

		//LOG("[*] The hypervisor has been installed.\n");

		return STATUS_SUCCESS;
	}
}
