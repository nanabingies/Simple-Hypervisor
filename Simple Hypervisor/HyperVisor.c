#pragma once
#include "stdafx.h"
#pragma warning(disable : 4996)

BOOLEAN VirtualizeAllProcessors() {

	//
	// This was more of an educational project so only one Logical Processor was chosen and virtualized
	// TODO : Add support for multiple processors
	// Fix: Support for multiple processors added
	//

	PROCESSOR_NUMBER processor_number;
	GROUP_AFFINITY affinity, old_affinity;

	ULONG numProcessors = KeQueryActiveProcessorCount(NULL);
	DbgPrint("[*] Current active processor count : %x\n", numProcessors);

	//
	// ExAllocatePool2 - Memory is zero initialized unless POOL_FLAG_UNINITIALIZED is specified.
	//
	vmm_context = (struct _vmm_context*)
		(ExAllocatePoolWithTag(NonPagedPool, sizeof(struct _vmm_context) * numProcessors, VMM_POOL));
	if (!vmm_context) {
		DbgPrint("[-] Failed to allocate pool for vmm_context\n");
		return FALSE;
	}

	for (unsigned iter = 0; iter < numProcessors; iter++) {
		KeGetProcessorNumberFromIndex(iter, &processor_number);

		RtlSecureZeroMemory(&affinity, sizeof(GROUP_AFFINITY));
		affinity.Group = processor_number.Group;
		affinity.Mask = (KAFFINITY)1 << processor_number.Number;
		KeSetSystemGroupAffinityThread(&affinity, &old_affinity);

		DbgPrint("[*] Currently executing on processor : %x\n", processor_number.Number);

		KIRQL irql = KeRaiseIrqlToDpcLevel();

		//
		// Check VMX Support for that Logical Processor
		//
		if (IsVmxSupport() == FALSE)	return FALSE;

		//
		// Check Bios Lock Bit
		//
		if (CheckBiosLock() == FALSE)	return FALSE;

		//
		// Enable VMXE for that processor
		//
		EnableCR4();

		DbgPrint("[*] Initial checks completed.\n");

		//
		// Allocate Memory for VMXON & VMCS regions and initialize
		//
		if (!allocateVmxonRegion(processor_number.Number))		return FALSE;
		if (!allocateVmcsRegion(processor_number.Number))		return FALSE;

		//
		// Allocate space for VM EXIT Handler
		//
		if (!allocateVmExitStack(processor_number.Number))		return FALSE;

		//
		// Allocate memory for IO Bitmap
		//
		if (!allocateIoBitmapStack(processor_number.Number))			return FALSE;

		//
		// Future: Add MSR Bitmap support
		//

		KeLowerIrql(irql);
		KeRevertToUserGroupAffinityThread(&old_affinity);
	}

	return TRUE;

}

VOID DevirtualizeAllProcessors() {

	ULONG numProcessors = KeQueryActiveProcessorCount(NULL);
	PROCESSOR_NUMBER processor_number;
	GROUP_AFFINITY affinity, old_affinity;

	for (unsigned iter = 0; iter < numProcessors; iter++) {
		KeGetProcessorNumberFromIndex(iter, &processor_number);

		RtlSecureZeroMemory(&affinity, sizeof(GROUP_AFFINITY));
		affinity.Group = processor_number.Group;
		affinity.Mask = (KAFFINITY)1 << processor_number.Number;
		KeSetSystemGroupAffinityThread(&affinity, &old_affinity);

		KIRQL irql = KeRaiseIrqlToDpcLevel();

		__vmx_vmclear(&vmm_context[processor_number.Number].vmcsRegionPhys);
		__vmx_off();

		if (vmm_context[processor_number.Number].vmcsRegionVirt)
			MmFreeContiguousMemory((PVOID)vmm_context[processor_number.Number].vmcsRegionVirt);

		if (vmm_context[processor_number.Number].vmxonRegionVirt)
			MmFreeContiguousMemory((PVOID)vmm_context[processor_number.Number].vmxonRegionVirt);

		if (vmm_context[processor_number.Number].HostStack)
			MmFreeContiguousMemory((PVOID)vmm_context[processor_number.Number].HostStack);

		if (vmm_context[processor_number.Number].bitmapAVirt)
			MmFreeContiguousMemory((PVOID)vmm_context[processor_number.Number].bitmapAVirt);

		if (vmm_context[processor_number.Number].bitmapBVirt)
			MmFreeContiguousMemory((PVOID)vmm_context[processor_number.Number].bitmapBVirt);
		
		ExFreePoolWithTag(vmm_context, VMM_POOL);

		KeLowerIrql(irql);
		KeRevertToUserGroupAffinityThread(&old_affinity);
	}

	return;
}


VOID LaunchVm(struct _KDPC* Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2) {

	__analysis_assume(DeferredContext != NULL);
	__analysis_assume(SystemArgument1 != NULL);
	__analysis_assume(SystemArgument2 != NULL);

	ULONG processorNumber = KeGetCurrentProcessorNumber();

	//
	// Set VMCS state to inactive
	//
	unsigned char retVal = __vmx_vmclear(&vmm_context[processorNumber].vmcsRegionPhys);
	if (retVal > 0) {
		DbgPrint("[-] VMCLEAR operation failed.\n");

		//__vmx_off();
		//VmOff = TRUE;
		return 0;
	}
	DbgPrint("[*] VMCS for processor %x set to inactive.\n", processorNumber);

	//
	//  Make VMCS the current and active on that processor
	//
	retVal = __vmx_vmptrld(&vmm_context[processorNumber].vmcsRegionPhys);
	if (retVal > 0) {
		DbgPrint("[-] VMPTRLD operation failed.\n");

		//__vmx_off();
		//VmOff = TRUE;
		return 0;
	}
	DbgPrint("[*] VMCS is current and active on processor %x\n", processorNumber);

	//
	// Setup VMCS structure fields for that logical processor
	//
	if (SetupVmcs(processorNumber) != VM_ERROR_OK) {
		DbgPrint("[-] Failure setting Virtual Machine VMCS.\n");

		size_t ErrorCode = 0;
		__vmx_vmread(VMCS_VM_INSTRUCTION_ERROR, &ErrorCode);
		DbgPrint("[-] Exiting with error code : %llx\n", ErrorCode);
		//__vmx_off();
		//VmOff = TRUE;
		return 0;
	}
	DbgPrint("[*] VMCS setup on processor %x done\n", processorNumber);

	//
	// Save HOST RSP & RBP
	//
	SaveHostRegisters(vmm_context[processorNumber].g_StackPointerForReturning, 
			vmm_context[processorNumber].g_BasePointerForReturning);

	//
	// Launch VM into Outer Space :)
	//
	__vmx_vmlaunch();

	DbgBreakPoint();
	KeSignalCallDpcSynchronize(SystemArgument2);
	KeSignalCallDpcDone(SystemArgument1);

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

	size_t Rip, InstLen;
	__vmx_vmread(VMCS_GUEST_RIP, &Rip);
	__vmx_vmread(VMCS_VMEXIT_INSTRUCTION_LENGTH, &InstLen);

	Rip += InstLen;
	__vmx_vmwrite(VMCS_GUEST_RIP, Rip);

	//
	// the VMRESUME instruction requires a VMCS whose launch state is  launched .
	// we're still in launch mode
	//
	__vmx_vmresume();

	return;
}
