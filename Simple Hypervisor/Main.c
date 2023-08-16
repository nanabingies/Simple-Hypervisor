#include "stdafx.h"
#pragma warning(disable : 4996)

VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject) {
	DbgPrint("[*] Terminating VMs on processors...\n");
	
	//if (VmOff == FALSE)
	//	DevirtualizeAllProcessors();

	if (DriverObject->DeviceObject != NULL)
		IoDeleteDevice(DriverObject->DeviceObject);

	DbgPrint("[*] Unloading Device Driver...\n");
	return;
}


NTSTATUS DefaultDispatch(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);

	DbgPrint("[*] Default Dispatch\n");
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}


NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);
	VmOff = FALSE;

	NTSTATUS status;
	UNICODE_STRING drvName;
	PDEVICE_OBJECT deviceObject;

	RtlInitUnicodeString(&drvName, DRV_NAME);

	status = IoCreateDevice(DriverObject, 0, &drvName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN,
		FALSE, (PDEVICE_OBJECT*)&deviceObject);
	if (!NT_SUCCESS(status))	return STATUS_FAILED_DRIVER_ENTRY;

	DbgPrint("[*] Successfully created device object\n");

	g_DriverGlobals = (struct driverGlobals*)ExAllocatePoolWithTag(NonPagedPool, sizeof(struct driverGlobals), VMM_POOL);
	if (!g_DriverGlobals) {
		DbgPrint("[-] DriverGlobals allocation failed.\n");
		IoDeleteDevice(deviceObject);
		return STATUS_FAILED_DRIVER_ENTRY;
	}

	g_DriverGlobals->g_DeviceObject = deviceObject;
	InitializeListHead(&g_DriverGlobals->g_ListofDevices);

	for (ULONG idx = 0; idx < IRP_MJ_MAXIMUM_FUNCTION; ++idx) {
		DriverObject->MajorFunction[idx] = DefaultDispatch;
	}
	DriverObject->DriverUnload = DriverUnload;

	//VirtualizeAllProcessors();
	//LaunchVm(0);

	CONTEXT ctx;
	RtlCaptureContext(&ctx);
	DbgPrint("[*] RAX : %llx\n", ctx.Rax);
	DbgPrint("[*] SegCs : %x\n", ctx.SegCs);
	DbgPrint("[*] SegDs : %x\n", ctx.SegDs);
	DbgPrint("[*] SegEs : %x\n", ctx.SegEs);
	DbgPrint("[*] SegFs : %x\n", ctx.SegFs);
	DbgPrint("[*] SegGs : %x\n", ctx.SegGs);
	DbgPrint("[*] SegSs : %x\n", ctx.SegSs);

	DbgPrint("[*] SegCs & 0xF8 : %x\n", (ctx.SegCs & 0xF8));
	DbgPrint("[*] SegDs & 0xF8 : %x\n", (ctx.SegDs & 0xF8));
	DbgPrint("[*] SegEs & 0xF8 : %x\n", (ctx.SegEs & 0xF8));
	DbgPrint("[*] SegFs & 0xF8 : %x\n", (ctx.SegFs & 0xF8));
	DbgPrint("[*] SegGs & 0xF8 : %x\n", (ctx.SegGs & 0xF8));
	DbgPrint("[*] SegSs & 0xF8 : %x\n", (ctx.SegSs & 0xF8));

	DbgPrint("[*] DR7 : %llx\n", __readdr(7));
	DbgPrint("[*] Eflags : %llx\n", __readeflags());

	__debugbreak();

	DbgPrint("[*] The hypervisor has been installed.\n");

	return STATUS_SUCCESS;
}