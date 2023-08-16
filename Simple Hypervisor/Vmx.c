#include "stdafx.h"

BOOLEAN IsVmxSupport() {
	DbgPrint("[*] Checking Processor VMX support.....\n");

	CPUID_EAX_01 args;
	__cpuid((int*)&args, 1);

	if (args.CpuidFeatureInformationEcx.VirtualMachineExtensions == 0) {
		DbgPrint("[-] This processor does not support VMX Extensions.\n");
		return FALSE;
	}

	DbgPrint("[*] This LP supports VMX extensions.\n");
	return TRUE;
}

VOID EnableCR4() {
	CR4 _cr4;

	_cr4.AsUInt = __readcr4();
	_cr4.VmxEnable = 1;

	__writecr4(_cr4.AsUInt);

	return;
}

BOOLEAN CheckBiosLock() {
	IA32_FEATURE_CONTROL_REGISTER feature_control_msr;
	feature_control_msr.AsUInt = __readmsr(IA32_FEATURE_CONTROL);

	if (feature_control_msr.LockBit == 0) {
		feature_control_msr.LockBit = 1;
		feature_control_msr.EnableVmxOutsideSmx = 1;

		__writemsr(IA32_FEATURE_CONTROL, feature_control_msr.AsUInt);
	}

	if (feature_control_msr.EnableVmxOutsideSmx == 0) {
		DbgPrint("[-] EnableVmxOutsideSmx bit not set.\n");
		return FALSE;
	}

	return TRUE;
}

VOID allocateVmxonRegion(UCHAR processorNumber) {
	if (!vmm_context) {
		DbgPrint("[-] Unspecified VM context for processor %x\n", processorNumber);
		return;
	}

	PHYSICAL_ADDRESS physAddr;
	physAddr.QuadPart = (LONGLONG)~0;

	IA32_VMX_BASIC_REGISTER vmx_basic;
	vmx_basic.AsUInt = __readmsr(IA32_VMX_BASIC);

	PVOID vmxon = MmAllocateContiguousMemory(vmx_basic.VmcsSizeInBytes, physAddr);
	if (!vmxon) {
		DbgPrint("[-] Allocating vmxon failed.\n");
		return;
	}

	RtlSecureZeroMemory(vmxon, vmx_basic.VmcsSizeInBytes);
	DbgPrint("[*] Allocated vmxon at %llx with size %llx\n",
		(UINT64)vmxon, vmx_basic.VmcsSizeInBytes);

	*(UINT64*)vmxon = vmx_basic.VmcsRevisionId;

	vmm_context->vmxonRegionVirt = (UINT64)vmxon;
	vmm_context->vmxonRegionPhys = VirtualToPhysicalAddress(vmxon);

	//
	// Execute VMXON
	//
	auto retVal = __vmx_on(&vmm_context->vmxonRegionPhys);
	if (retVal > 0) {
		DbgPrint("[-] Failed vmxon with error code %x\n", retVal);
		return;
	}

	DbgPrint("[*] vmxon initialized on logical processor %x\n", processorNumber);
	return;
}


VOID allocateVmcsRegion(UCHAR processorNumber) {
	if (!vmm_context) {
		DbgPrint("[-] Unspecified VM context for processor %x\n", processorNumber);
		return;
	}

	PHYSICAL_ADDRESS physAddr;
	physAddr.QuadPart = (LONGLONG)~0;

	IA32_VMX_BASIC_REGISTER vmx_basic;
	vmx_basic.AsUInt = __readmsr(IA32_VMX_BASIC);

	PVOID vmcs = MmAllocateContiguousMemory(vmx_basic.VmcsSizeInBytes, physAddr);
	if (!vmcs) {
		DbgPrint("[-] Allocating vmcs failed.\n");
		return;
	}

	RtlSecureZeroMemory(vmcs, vmx_basic.VmcsSizeInBytes);
	DbgPrint("[*] Allocated vmcs at %llx with size %llx\n",
		(UINT64)vmcs, vmx_basic.VmcsSizeInBytes);

	*(UINT64*)vmcs = vmx_basic.VmcsRevisionId;

	vmm_context->vmcsRegionVirt = (UINT64)vmcs;
	vmm_context->vmcsRegionPhys = VirtualToPhysicalAddress(vmcs);

	//
	// Load current VMCS and make it active
	//
	auto retVal = __vmx_vmptrld(&vmm_context->vmcsRegionPhys);
	if (retVal > 0) {
		DbgPrint("[-] Failed vmcs with error code %x\n", retVal);
		return;
	}

	DbgPrint("[*] vmcs loaded on logical processor %x\n", processorNumber);
	return;
}