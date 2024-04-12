#include "vmx.hpp"
#include "stdafx.h"
#include "logger.hpp"

extern "C" {

	auto DriverUnload(_In_ PDRIVER_OBJECT driver_object) -> void {
		UNREFERENCED_PARAMETER(driver_object);
		return;
	}

	auto DefaultDispatch(_In_ PDEVICE_OBJECT, _In_ PIRP irp) -> NTSTATUS {
		irp->IoStatus.Status = STATUS_SUCCESS;
		irp->IoStatus.Information = 0;

		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	unsigned g_num_processors;

	auto DriverEntry(_In_ PDRIVER_OBJECT, _In_ PUNICODE_STRING) -> NTSTATUS {
		using hv_vmcs::init_vmcs;
		using hv::launch_vm;
		using vmx::create_vcpus;

		// Opt-in to using non-executable pool memory on Windows 8 and later.
		// https://msdn.microsoft.com/en-us/library/windows/hardware/hh920402(v=vs.85).aspx
		ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

		if (!create_vcpus())	return STATUS_FAILED_DRIVER_ENTRY;

		auto cr3_val = __readcr3();
		KeIpiGenericCall(PKIPI_BROADCAST_WORKER(&init_vmcs) , cr3_val);
		KeIpiGenericCall(PKIPI_BROADCAST_WORKER(launch_vm), 0);

		return STATUS_SUCCESS;
	}
}
