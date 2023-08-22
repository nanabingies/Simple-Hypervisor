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

BOOLEAN allocateVmxonRegion(UCHAR processorNumber) {
	if (!vmm_context) {
		DbgPrint("[-] Unspecified VM context for processor %x\n", processorNumber);
		return FALSE;
	}

	PHYSICAL_ADDRESS physAddr;
	physAddr.QuadPart = (LONGLONG)~0;

	IA32_VMX_BASIC_REGISTER vmx_basic;
	vmx_basic.AsUInt = __readmsr(IA32_VMX_BASIC);

	PVOID vmxon = MmAllocateContiguousMemory(vmx_basic.VmcsSizeInBytes, physAddr);
	if (!vmxon) {
		DbgPrint("[-] Allocating vmxon failed.\n");
		return FALSE;
	}

	RtlSecureZeroMemory(vmxon, vmx_basic.VmcsSizeInBytes);
	DbgPrint("[*] Allocated vmxon at %llx with size %llx\n",
		(UINT64)vmxon, vmx_basic.VmcsSizeInBytes);

	*(UINT64*)vmxon = vmx_basic.VmcsRevisionId;

	vmm_context[processorNumber].vmxonRegionVirt = (UINT64)vmxon;
	vmm_context[processorNumber].vmxonRegionPhys = VirtualToPhysicalAddress(vmxon);

	//
	// Execute VMXON
	//
	auto retVal = __vmx_on(&vmm_context[processorNumber].vmxonRegionPhys);
	if (retVal > 0) {
		DbgPrint("[-] Failed vmxon with error code %x\n", retVal);
		return FALSE;
	}

	DbgPrint("[*] vmxon initialized on logical processor %x\n", processorNumber);
	return TRUE;
}


BOOLEAN allocateVmcsRegion(UCHAR processorNumber) {
	if (!vmm_context) {
		DbgPrint("[-] Unspecified VM context for processor %x\n", processorNumber);
		return FALSE;
	}

	PHYSICAL_ADDRESS physAddr;
	physAddr.QuadPart = (LONGLONG)~0;

	IA32_VMX_BASIC_REGISTER vmx_basic;
	vmx_basic.AsUInt = __readmsr(IA32_VMX_BASIC);

	PVOID vmcs = MmAllocateContiguousMemory(vmx_basic.VmcsSizeInBytes, physAddr);
	if (!vmcs) {
		DbgPrint("[-] Allocating vmcs failed.\n");
		return FALSE;
	}

	RtlSecureZeroMemory(vmcs, vmx_basic.VmcsSizeInBytes);
	DbgPrint("[*] Allocated vmcs at %llx with size %llx\n",
		(UINT64)vmcs, vmx_basic.VmcsSizeInBytes);

	*(UINT64*)vmcs = vmx_basic.VmcsRevisionId;

	vmm_context[processorNumber].vmcsRegionVirt = (UINT64)vmcs;
	vmm_context[processorNumber].vmcsRegionPhys = VirtualToPhysicalAddress(vmcs);

	//
	// Load current VMCS and make it active
	//
	auto retVal = __vmx_vmptrld(&vmm_context[processorNumber].vmcsRegionPhys);
	if (retVal > 0) {
		DbgPrint("[-] Failed vmcs with error code %x\n", retVal);
		return FALSE;
	}

	DbgPrint("[*] vmcs loaded on logical processor %x\n", processorNumber);
	return TRUE;
}

BOOLEAN allocateVmExitStack(UCHAR processorNumber) {
	PHYSICAL_ADDRESS physAddr;
	physAddr.QuadPart = (ULONGLONG)~0;
	PVOID vmexitStack = MmAllocateContiguousMemory(STACK_SIZE, physAddr);
	if (!vmexitStack) {
		DbgPrint("[-] Failure allocating memory for VM EXIT Handler.\n");
		return FALSE;
	}
	RtlSecureZeroMemory(vmexitStack, STACK_SIZE);

	vmm_context[processorNumber].HostStack = (UINT64)vmexitStack;
	DbgPrint("[*] vmm_context[processorNumber].HostStack : %llx\n", vmm_context[processorNumber].HostStack);

	return TRUE;
}

BOOLEAN allocateIoBitmapStack(UCHAR processorNumber) {
	PHYSICAL_ADDRESS physAddr;
	physAddr.QuadPart = (ULONGLONG)~0;
	PVOID bitmap = MmAllocateContiguousMemory(PAGE_SIZE, physAddr);
	if (!bitmap) {
		DbgPrint("[-] Failure allocating memory for IO Bitmap A.\n");
		return FALSE;
	}
	RtlSecureZeroMemory(bitmap, PAGE_SIZE);

	vmm_context[processorNumber].bitmapAVirt = (UINT64)bitmap;
	vmm_context[processorNumber].bitmapAPhys = VirtualToPhysicalAddress(bitmap);

	physAddr.QuadPart = (ULONGLONG)~0;
	bitmap = MmAllocateContiguousMemory(PAGE_SIZE, physAddr);
	if (!bitmap) {
		DbgPrint("[-] Failure allocating memory for IO Bitmap B.\n");
		return FALSE;
	}
	RtlSecureZeroMemory(bitmap, PAGE_SIZE);

	vmm_context[processorNumber].bitmapBVirt = (UINT64)bitmap;
	vmm_context[processorNumber].bitmapBPhys = VirtualToPhysicalAddress(bitmap);

	DbgPrint("[*] vmm_context[processorNumber].bitmapAVirt : %llx\n", vmm_context[processorNumber].bitmapAVirt);
	DbgPrint("[*] vmm_context[processorNumber].bitmapBVirt : %llx\n", vmm_context[processorNumber].bitmapBVirt);

	return TRUE;
}