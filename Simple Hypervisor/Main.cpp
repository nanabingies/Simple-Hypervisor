#include "vmx.hpp"
#include "stdafx.h"

extern "C" {

	auto DriverUnload(_In_ PDRIVER_OBJECT driver_object) -> void {
		LOG("[*] Terminating VMs on processors...");

		// Uninstall vmx on all processors
		if (!VmOff) {
			hv::DevirtualizeAllProcessors();
			VmOff = true;
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

	auto DriverEntry(_In_ PDRIVER_OBJECT driver_object, _In_ PUNICODE_STRING registry_path) -> NTSTATUS {
		LOG("[*] Loading file %wZ", registry_path);

		VmOff = false;

		// Opt-in to using non-executable pool memory on Windows 8 and later.
		// https://msdn.microsoft.com/en-us/library/windows/hardware/hh920402(v=vs.85).aspx
		ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

		g_num_processors = KeQueryActiveProcessorCount(NULL);
		LOG("[*] Current active processors : %x\n", g_num_processors);

		NTSTATUS status;
		UNICODE_STRING drv_name;
		PDEVICE_OBJECT device_object;

		RtlInitUnicodeString(&drv_name, DRV_NAME);

		status = IoCreateDevice(driver_object, 0, &drv_name, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN,
			FALSE, reinterpret_cast<PDEVICE_OBJECT*>(&device_object));
		if (!NT_SUCCESS(status))	return STATUS_FAILED_DRIVER_ENTRY;

		LOG("[*] Successfully created device object.");

		for (ulong idx = 0; idx < IRP_MJ_MAXIMUM_FUNCTION; ++idx) {
			driver_object->MajorFunction[idx] = DefaultDispatch;
		}
		driver_object->DriverUnload = DriverUnload;

		if (!vmx::VmxIsVmxAvailable())	return STATUS_FAILED_DRIVER_ENTRY;

		if (!hv::VirtualizeAllProcessors())	return STATUS_FAILED_DRIVER_ENTRY;

		LOG("[*] The hypervisor has been installed.");

		return STATUS_SUCCESS;
	}
}
