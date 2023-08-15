#pragma once
#include "stdafx.h"
#pragma warning(disable : 4996)

VOID VirtualizeAllProcessors() {
	//
	// This was more of an educational project so only one Logical Processor was chosen and virtualized
	// TODO : Add support for multiple processors
	//

	ULONG procNumber = 0;
	PROCESSOR_NUMBER processor_number;
	GROUP_AFFINITY affinity, old_affinity;

	ULONG numProcessors = KeQueryActiveProcessorCount(NULL);
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

VOID DevirtualizeAllProcessors() {
	ULONG procNumber = 0;
	PROCESSOR_NUMBER processor_number;
	GROUP_AFFINITY affinity, old_affinity;

	KeGetProcessorNumberFromIndex(procNumber, &processor_number);

	RtlSecureZeroMemory(&affinity, sizeof(GROUP_AFFINITY));
	affinity.Group = processor_number.Group;
	affinity.Mask = (KAFFINITY)1 << processor_number.Number;
	KeSetSystemGroupAffinityThread(&affinity, &old_affinity);

	__vmx_vmclear(&vmm_context->vmcsRegionPhys);
	__vmx_off();
	MmFreeContiguousMemory((PVOID)vmm_context->vmcsRegionVirt);
	MmFreeContiguousMemory((PVOID)vmm_context->vmxonRegionVirt);

	KeRevertToUserGroupAffinityThread(&old_affinity);
}

VOID LaunchVm(int processorId) {
	//
	// Bind to processor
	// 
	PROCESSOR_NUMBER processor_number;
	GROUP_AFFINITY affinity, old_affinity;

	KeGetProcessorNumberFromIndex(processorId, &processor_number);

	RtlSecureZeroMemory(&affinity, sizeof(GROUP_AFFINITY));
	affinity.Group = processor_number.Group;
	affinity.Mask = (KAFFINITY)1 << processor_number.Number;
	KeSetSystemGroupAffinityThread(&affinity, &old_affinity);

	//
	// Allocate space for VM EXIT Handler
	//
	PVOID vmexitHandler = ExAllocatePoolWithTag(NonPagedPool, STACK_SIZE, VMM_POOL);
	if (!vmexitHandler) {
		DbgPrint("[-] Failure allocating memory for VM EXIT Handler.\n");
		return;
	}
	RtlSecureZeroMemory(vmexitHandler, STACK_SIZE);
	vmm_context->GuestStack = (UINT64)vmexitHandler;

	//
	// Allocate memory for MSR Bitmap
	//
	PHYSICAL_ADDRESS physAddr;
	physAddr.QuadPart = (ULONGLONG)~0;
	PVOID bitmap = MmAllocateContiguousMemory(PAGE_SIZE, physAddr);
	if (!bitmap) {
		DbgPrint("[-] Failure allocating memory for MSR Bitmap.\n");
		ExFreePoolWithTag(vmexitHandler, VMM_POOL);

		__vmx_off();
		return;
	}
	RtlSecureZeroMemory(bitmap, PAGE_SIZE);

	vmm_context->bitmapVirt = (UINT64)bitmap;
	vmm_context->bitmapPhys = VirtualToPhysicalAddress(bitmap);

	//
	// Set VMCS state to inactive
	//
	unsigned char retVal = __vmx_vmclear(&vmm_context->vmcsRegionPhys);
	if (retVal > 0) {
		DbgPrint("[-] VMCLEAR operation failed.\n");

		__vmx_off();
		return;
	}

	//
	//  Make VMCS the current and active 
	//
	retVal = __vmx_vmptrld(&vmm_context->vmcsRegionPhys);
	if (retVal > 0) {
		DbgPrint("[-] VMPTRLD operation failed.\n");

		__vmx_off();
		return;
	}

	//
	// Setup VMCS structure fields
	//
	if (SetupVmcs() != VM_ERROR_OK) {
		DbgPrint("[-] Failure setting Virtual Machine VMCS.\n");

		ULONG64 ErrorCode = 0;
		__vmx_vmread(VMCS_VM_INSTRUCTION_ERROR, &ErrorCode);
		__vmx_off();
		return;
	}

	//
	// Save HOST RSP & RBP
	//
	SaveStackRegs();

	//
	// Launch VM into Outer Space :)
	//
	retVal = __vmx_vmlaunch();
	if (retVal > 0) {
		DbgPrint("[-] VMLAUNCH operation failed with error code %x.\n", retVal);

		//
		// if VMLAUNCH succeeds will never be here!
		//
		ULONG64 ErrorCode = 0;
		__vmx_vmread(VMCS_VM_INSTRUCTION_ERROR, &ErrorCode);
		__vmx_off();
		DbgPrint("[*] VMLAUNCH Error : 0x%llx\n", ErrorCode);
		VmOff = TRUE;
		DbgBreakPoint();
		return;
	}

	DbgPrint("[*] VMLAUNCH operation successful.\n");

	KeRevertToUserGroupAffinityThread(&old_affinity);

	return;
}

VOID TerminateVm() {

	//
	// Set VMCS state to inactive
	//
	__vmx_vmclear(&vmm_context->vmcsRegionPhys);

	return;
}

VOID ResumeVm() {

	//
	// the VMRESUME instruction requires a VMCS whose launch state is  launched .
	// we're still in launch mode
	//
	__vmx_vmresume();

	return;
}

VOID VmExit() {
	return;
}