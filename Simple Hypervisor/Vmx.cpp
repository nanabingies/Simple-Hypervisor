#include "stdafx.h"

auto VmxIsVmxAvailable() -> bool {
	PAGED_CODE();

	//
	// Check VMX Support for that Logical Processor
	//
	if (!VmxIsVmxSupport())	return false;

	//
	// Check Bios Lock Bit
	//
	if (!VmxCheckBiosLock())	return false;

	//
	// Enable VMXE for that processor
	//
	VmxEnableCR4();

	//
	// Check for EPT support for that processor
	//
	if (!CheckEPTSupport())	return false;


	DbgPrint("[*] Initial checks completed.\n");

	//
	// Build MTRR Map
	//
	if (!EptBuildMTRRMap())	return false;
	DbgPrint("[*] MTRR built successfully\n");

	return TRUE;
}

auto VmxIsVmxSupport() -> bool {
	PAGED_CODE();

	DbgPrint("[*] Checking Processor VMX support.....\n");

	cpuid_eax_01 args;
	__cpuid((int*)&args, 1);

	if (args.cpuid_feature_information_ecx.virtual_machine_extensions == 0) {
		DbgPrint("[-] This processor does not support VMX Extensions.\n");
		return false;
	}

	DbgPrint("[*] This LP supports VMX extensions.\n");
	return true;
}

auto VmxEnableCR4() -> void {
	PAGED_CODE();

	cr4 _cr4;

	_cr4.flags = __readcr4();
	_cr4.vmx_enable = 1;

	__writecr4(_cr4.flags);

	return;
}

auto VmxCheckBiosLock() -> bool {
	PAGED_CODE();

	ia32_feature_control_register feature_control_msr;
	feature_control_msr.flags = __readmsr(IA32_FEATURE_CONTROL);

	if (feature_control_msr.lock_bit == 0) {
		feature_control_msr.lock_bit = 1;
		feature_control_msr.enable_vmx_outside_smx = 1;

		__writemsr(IA32_FEATURE_CONTROL, feature_control_msr.flags);
	}

	if (feature_control_msr.enable_vmx_outside_smx == 0) {
		DbgPrint("[-] EnableVmxOutsideSmx bit not set.\n");
		return false;
	}

	return true;
}


auto VmxAllocateVmxonRegion(uchar processorNumber) -> bool {
	PAGED_CODE();

	if (!vmm_context) {
		DbgPrint("[-] Unspecified VM context for processor %x\n", processorNumber);
		return false;
	}

	PHYSICAL_ADDRESS physAddr;
	physAddr.QuadPart = (LONGLONG)~0;

	ia32_vmx_basic_register vmx_basic;
	vmx_basic.flags = __readmsr(IA32_VMX_BASIC);

	void* vmxon = MmAllocateContiguousMemory(vmx_basic.vmcs_size_in_bytes, physAddr);
	if (!vmxon) {
		DbgPrint("[-] Allocating vmxon failed.\n");
		return false;
	}

	RtlSecureZeroMemory(vmxon, vmx_basic.vmcs_size_in_bytes);
	DbgPrint("[*] Allocated vmxon at %llx with size %llx\n",
		reinterpret_cast<uint64_t>(vmxon), vmx_basic.vmcs_size_in_bytes);

	*reinterpret_cast<uint64_t*>(vmxon) = vmx_basic.vmcs_revision_id;

	vmm_context[processorNumber].vmxonRegionVirt = reinterpret_cast<uint64_t>(vmxon);
	vmm_context[processorNumber].vmxonRegionPhys = VirtualToPhysicalAddress(vmxon);

	//
	// Execute VMXON
	//
	auto retVal = __vmx_on(&vmm_context[processorNumber].vmxonRegionPhys);
	if (retVal > 0) {
		DbgPrint("[-] Failed vmxon with error code %x\n", retVal);
		return false;
	}

	DbgPrint("[*] vmxon initialized on logical processor %x\n", processorNumber);
	return true;
}


auto VmxAllocateVmcsRegion(uchar processorNumber) -> bool {
	PAGED_CODE();

	if (!vmm_context) {
		DbgPrint("[-] Unspecified VM context for processor %x\n", processorNumber);
		return false;
	}

	PHYSICAL_ADDRESS physAddr;
	physAddr.QuadPart = (LONGLONG)~0;

	ia32_vmx_basic_register vmx_basic;
	vmx_basic.flags = __readmsr(IA32_VMX_BASIC);

	void* vmcs = MmAllocateContiguousMemory(vmx_basic.vmcs_size_in_bytes, physAddr);
	if (!vmcs) {
		DbgPrint("[-] Allocating vmcs failed.\n");
		return false;
	}

	RtlSecureZeroMemory(vmcs, vmx_basic.vmcs_size_in_bytes);
	DbgPrint("[*] Allocated vmcs at %llx with size %llx\n",
		reinterpret_cast<uint64_t>(vmcs), vmx_basic.vmcs_size_in_bytes);

	*reinterpret_cast<uint64_t*>(vmcs) = vmx_basic.vmcs_size_in_bytes;

	vmm_context[processorNumber].vmcsRegionVirt = reinterpret_cast<uint64_t>(vmcs);
	vmm_context[processorNumber].vmcsRegionPhys = VirtualToPhysicalAddress(vmcs);

	//
	// Load current VMCS and make it active
	//
	auto retVal = __vmx_vmptrld(&vmm_context[processorNumber].vmcsRegionPhys);
	if (retVal > 0) {
		DbgPrint("[-] Failed vmcs with error code %x\n", retVal);
		return false;
	}

	DbgPrint("[*] vmcs loaded on logical processor %x\n", processorNumber);
	return true;
}


auto VmxAllocateVmExitStack(uchar processorNumber) -> bool {
	PAGED_CODE();

	PHYSICAL_ADDRESS physAddr;
	physAddr.QuadPart = static_cast<ULONGLONG>(~0);

	void* vmexitStack = MmAllocateContiguousMemory(HOST_STACK_SIZE, physAddr);
	if (!vmexitStack) {
		DbgPrint("[-] Failure allocating memory for VM EXIT Handler.\n");
		return false;
	}
	RtlSecureZeroMemory(vmexitStack, HOST_STACK_SIZE);

	vmm_context[processorNumber].HostStack = reinterpret_cast<uint64_t>(vmexitStack);
	DbgPrint("[*] vmm_context[processorNumber].HostStack : %llx\n", vmm_context[processorNumber].HostStack);

	return true;
}


auto VmxAllocateIoBitmapStack(uchar processorNumber) -> bool {
	PAGED_CODE();

	PHYSICAL_ADDRESS physAddr;
	physAddr.QuadPart = static_cast<ULONGLONG>(~0);

	void* bitmap = MmAllocateContiguousMemory(PAGE_SIZE, physAddr);
	if (!bitmap) {
		DbgPrint("[-] Failure allocating memory for IO Bitmap A.\n");
		return false;
	}
	RtlSecureZeroMemory(bitmap, PAGE_SIZE);

	vmm_context[processorNumber].ioBitmapAVirt = reinterpret_cast<uint64_t>(bitmap);
	vmm_context[processorNumber].ioBitmapAPhys = VirtualToPhysicalAddress(bitmap);

	physAddr.QuadPart = static_cast<ULONGLONG>(~0);
	bitmap = MmAllocateContiguousMemory(PAGE_SIZE, physAddr);
	if (!bitmap) {
		DbgPrint("[-] Failure allocating memory for IO Bitmap B.\n");
		return false;
	}
	RtlSecureZeroMemory(bitmap, PAGE_SIZE);

	vmm_context[processorNumber].ioBitmapBVirt = reinterpret_cast<uint64_t>(bitmap);
	vmm_context[processorNumber].ioBitmapBPhys = VirtualToPhysicalAddress(bitmap);

	DbgPrint("[*] vmm_context[processorNumber].bitmapAVirt : %llx\n", vmm_context[processorNumber].ioBitmapAVirt);
	DbgPrint("[*] vmm_context[processorNumber].bitmapBVirt : %llx\n", vmm_context[processorNumber].ioBitmapBVirt);

	//
	// We want to vmexit on every io and msr access
	//memset(vmm_context[processorNumber].ioBitmapAVirt, 0xff, PAGE_SIZE);
	//memset(vmm_context[processorNumber].ioBitmapBVirt, 0xff, PAGE_SIZE);

	return true;
}


auto VmxAllocateMsrBitmap(UCHAR processorNumber) -> bool {
	PAGED_CODE();

	PHYSICAL_ADDRESS physAddr;
	physAddr.QuadPart = static_cast<ULONGLONG>(~0);

	void* bitmap = MmAllocateContiguousMemory(PAGE_SIZE, physAddr);
	if (!bitmap) {
		DbgPrint("[-] Failure allocating memory for IO Bitmap A.\n");
		return false;
	}
	RtlSecureZeroMemory(bitmap, PAGE_SIZE);

	vmm_context[processorNumber].msrBitmapVirt = reinterpret_cast<uint64_t>(bitmap);
	vmm_context[processorNumber].msrBitmapPhys = VirtualToPhysicalAddress(bitmap);

	return true;
}
