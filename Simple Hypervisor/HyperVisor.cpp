#pragma once
#include "stdafx.h"
#pragma warning(disable : 4996)

BOOLEAN VirtualizeAllProcessors() {
	PAGED_CODE();

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
		// Allocate Memory for VMXON & VMCS regions and initialize
		//
		if (!VmxAllocateVmxonRegion(processor_number.Number))		return FALSE;
		if (!VmxAllocateVmcsRegion(processor_number.Number))		return FALSE;

		//
		// Allocate space for VM EXIT Handler
		//
		if (!VmxAllocateVmExitStack(processor_number.Number))		return FALSE;

		//
		// Allocate memory for IO Bitmap
		//
		if (!VmxAllocateIoBitmapStack(processor_number.Number))			return FALSE;

		//
		// Future: Add MSR Bitmap support
		// Fix: Added MSR Bitmap support
		//
		if (!VmxAllocateMsrBitmap(processor_number.Number))			return FALSE;

		//
		// Setup EPT support for that processor
		//
		if (!InitializeEpt(processor_number.Number))				return FALSE;

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

		if (vmm_context[processor_number.Number].ioBitmapAVirt)
			MmFreeContiguousMemory((PVOID)vmm_context[processor_number.Number].ioBitmapAVirt);

		if (vmm_context[processor_number.Number].ioBitmapBVirt)
			MmFreeContiguousMemory((PVOID)vmm_context[processor_number.Number].ioBitmapBVirt);

		if (vmm_context[processor_number.Number].msrBitmapVirt)
			MmFreeContiguousMemory((PVOID)vmm_context[processor_number.Number].msrBitmapVirt);

		if (vmm_context[processor_number.Number].EptState) {
			if (vmm_context[processor_number.Number].EptState->EptPageTable)
				ExFreePoolWithTag(vmm_context[processor_number.Number].EptState->EptPageTable, VMM_POOL);

			if (vmm_context[processor_number.Number].EptState->EptPtr)
				ExFreePoolWithTag(vmm_context[processor_number.Number].EptState->EptPtr, VMM_POOL);

			ExFreePoolWithTag(vmm_context[processor_number.Number].EptState, VMM_POOL);
		}
		
		ExFreePoolWithTag(vmm_context, VMM_POOL);

		KeLowerIrql(irql);
		KeRevertToUserGroupAffinityThread(&old_affinity);
	}

	return;
}


ULONG_PTR LaunchVm(ULONG_PTR Argument) {

	ULONG processorNumber = KeGetCurrentProcessorNumber();

	//
	// Set VMCS state to inactive
	//
	unsigned char retVal = __vmx_vmclear(&vmm_context[processorNumber].vmcsRegionPhys);
	if (retVal > 0) {
		DbgPrint("[-] VMCLEAR operation failed.\n");
		return Argument;
	}
	DbgPrint("[*] VMCS for processor %x set to inactive.\n", processorNumber);

	//
	//  Make VMCS the current and active on that processor
	//
	retVal = __vmx_vmptrld(&vmm_context[processorNumber].vmcsRegionPhys);
	if (retVal > 0) {
		DbgPrint("[-] VMPTRLD operation failed.\n");
		return Argument;
	}
	DbgPrint("[*] VMCS is current and active on processor %x\n", processorNumber);

	//
	// Setup VMCS structure fields for that logical processor
	//
	// SetupVmcs(processorNumber)
	if (AsmSetupVmcs(processorNumber) != VM_ERROR_OK) {
		DbgPrint("[-] Failure setting Virtual Machine VMCS.\n");

		size_t ErrorCode = 0;
		__vmx_vmread(VMCS_VM_INSTRUCTION_ERROR, &ErrorCode);
		DbgPrint("[-] Exiting with error code : %llx\n", ErrorCode);
		return Argument;
	}
	DbgPrint("[*] VMCS setup on processor %x done\n", processorNumber);

	

	//
	// Launch VM into Outer Space :)
	//
	__vmx_vmlaunch();
	
	// We should never get here
	DbgBreakPoint();
	DbgPrint("[-] Failure launching Virtual Machine.\n");

	size_t ErrorCode = 0;
	__vmx_vmread(VMCS_VM_INSTRUCTION_ERROR, &ErrorCode);
	DbgPrint("[-] Exiting with error code : %llx\n", ErrorCode);

	return Argument;
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
	// the VMRESUME instruction requires a VMCS whose launch state is set to launched .
	// we're still in launch mode
	//
	__vmx_vmresume();

	return;
}
