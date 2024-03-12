#include "vmx.hpp"
#include "stdafx.h"
#include "logger.hpp"

extern "C" {

	auto DriverUnload(_In_ PDRIVER_OBJECT driver_object) -> void {
		using hv::devirtualize_all_processors;

		LOG("[*] Terminating VMs on processors...");

		// Uninstall vmx on all processors
		if (!vm_off) {
			devirtualize_all_processors();
			vm_off = true;
		}

		if (driver_object->DeviceObject != nullptr) {
			IoDeleteDevice(driver_object->DeviceObject);
		}

		LOG("[*] Unloading Device Driver...");
		return;
	}

	auto DefaultDispatch(_In_ PDEVICE_OBJECT, _In_ PIRP irp) -> NTSTATUS {
		LOG("[*] Default Dispatch");

		irp->IoStatus.Status = STATUS_SUCCESS;
		irp->IoStatus.Information = 0;

		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	bool vm_off;
	unsigned g_num_processors;

	auto DriverEntry(_In_ PDRIVER_OBJECT driver_object, _In_ PUNICODE_STRING registry_path) -> NTSTATUS {
		using hv::virtualize_all_processors;
		using hv::launch_vm;
		using hv::launch_all_vmms;
		using vmx::vmx_is_vmx_available;

		LOG("[*] Loading file %wZ", registry_path);

		vm_off = false;

		// Opt-in to using non-executable pool memory on Windows 8 and later.
		// https://msdn.microsoft.com/en-us/library/windows/hardware/hh920402(v=vs.85).aspx
		ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

		g_num_processors = KeQueryActiveProcessorCount(NULL);
		LOG("[*] Current active processors : %x\n", g_num_processors);

		//LOG("[*] Num processors : %x\n", KeNumberProcessors);

		NTSTATUS status;
		UNICODE_STRING drv_name;
		PDEVICE_OBJECT device_object;

		RtlInitUnicodeString(&drv_name, DRV_NAME);

		status = IoCreateDevice(driver_object, 0, &drv_name, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN,
			FALSE, reinterpret_cast<PDEVICE_OBJECT*>(&device_object));
		if (!NT_SUCCESS(status))	return STATUS_FAILED_DRIVER_ENTRY;

		LOG("[*] Successfully created device object.\n");

		for (unsigned idx = 0; idx < IRP_MJ_MAXIMUM_FUNCTION; ++idx) {
			driver_object->MajorFunction[idx] = DefaultDispatch;
		}
		driver_object->DriverUnload = DriverUnload;

		if (!vmx_is_vmx_available())	return STATUS_FAILED_DRIVER_ENTRY;

		if (!virtualize_all_processors())	return STATUS_FAILED_DRIVER_ENTRY;

		//KeIpiGenericCall(static_cast<PKIPI_BROADCAST_WORKER>(launch_vm), 0);
		launch_all_vmms();

		LOG("[*] The hypervisor has been installed.\n");

		return STATUS_SUCCESS;
	}
}
