#pragma once
#include "stdafx.h"

VOID VirtualizeAllProcessors() {
	//
	// This was more of an educational project so only one Logical Processor was chosen and virtualized
	// TODO : Add support for multiple processors
	//

	auto procNumber = 0;
	PROCESSOR_NUMBER processor_number;
	GROUP_AFFINITY affinity, old_affinity;

	numProcessors = KeQueryActiveProcessorCount(NULL);
	DbgPrint("[*] Current active processor count : %x\n", numProcessors);

	//
	// ExAllocatePool2 - Memory is zero initialized unless POOL_FLAG_UNINITIALIZED is specified.
	//
	vmm_context = (struct _vmm_context*)
		(ExAllocatePoolWithTag(NonPagedPool, sizeof(struct _vmm_context), VMM_POOL));
	if (!vmm_context) {
		DbgPrint("[-] Failed to allocate pool for vmm_context\n");
		return;
	}

	KeGetProcessorNumberFromIndex(procNumber, &processor_number);

	RtlSecureZeroMemory(&affinity, sizeof(GROUP_AFFINITY));
	affinity.Group = processor_number.Group;
	affinity.Mask = (KAFFINITY)1 << processor_number.Number;
	KeSetSystemGroupAffinityThread(&affinity, &old_affinity);

	DbgPrint("[*] Currently executing on processor : %x\n", processor_number.Number);

	//
	// Check VMX Support for that Logical Processor
	//
	if (IsVmxSupport() == FALSE)	return;

	//
	// Check Bios Lock Bit
	//
	if (CheckBiosLock() == FALSE)	return;

	//
	// Enable VMXE for that processor
	//
	EnableCR4();
	DbgPrint("[*] Initial checks completed.\n");

	//
	// Allocate Memory for VMXON & VMCS regions and initialize
	//
	allocateVmxonRegion(processor_number.Number);
	allocateVmcsRegion(processor_number.Number);

	//
	// Setup EPT structures
	//
	InitializeEpt();

	KeRevertToUserGroupAffinityThread(&old_affinity);
}