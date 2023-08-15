#include "stdafx.h"

VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject) {
	DbgPrint("[*] Terminating VMs on processors...\n");
	
	DevirtualizeAllProcessors();

	UNICODE_STRING usString;
	RtlInitUnicodeString(&usString, DOS_NAME);
	IoDeleteSymbolicLink(static_cast<PUNICODE_STRING>(&usString));

	if (DriverObject->DeviceObject != nullptr)
		IoDeleteDevice(DriverObject->DeviceObject);

	DbgPrint("[*] Unloading Device Driver...\n");
	return;
}