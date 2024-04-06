#include "stdafx.h"
_vmm_context* vmm_context;

namespace vmx {
	auto vmx_is_vmx_available() -> bool {
		PAGED_CODE();

		//
		// Check VMX Support for all Logical Processors
		//
		if (!vmx_is_vmx_support())	return false;

		//
		// Check Bios Lock Bit
		//
		if (!vmx_check_bios_lock())	return false;

		//
		// Enable VMXE for all processors
		//
		vmx_enable_cr4();

		//
		// Check for EPT support for all processors
		//
		using ept::check_ept_support;
		if (!check_ept_support())	return false;


		//LOG("[*] Initial checks completed.\n");

		//
		// Build MTRR Map
		//
		//if (EptBuildMTRRMap() == FALSE)	return FALSE;
		//LOG("[*] MTRR built successfully\n");

		return TRUE;
	}

	auto vmx_is_vmx_support() -> bool {
		PAGED_CODE();

		PROCESSOR_NUMBER processor_number;
		GROUP_AFFINITY affinity, old_affinity;

		for (unsigned iter = 0; iter < g_num_processors; iter++) {
			KeGetProcessorNumberFromIndex(iter, &processor_number);

			RtlSecureZeroMemory(&affinity, sizeof(GROUP_AFFINITY));
			affinity.Group = processor_number.Group;
			affinity.Mask = static_cast<KAFFINITY>(1) << processor_number.Number;
			KeSetSystemGroupAffinityThread(&affinity, &old_affinity);

			auto irql = KeRaiseIrqlToDpcLevel();

			LOG("[*] Checking processor (%x) VMX support.....\n", processor_number.Number);
			cpuid_eax_01 args{};
			__cpuid(reinterpret_cast<int32_t*>(&args), 1);

			if (args.cpuid_feature_information_ecx.virtual_machine_extensions == 0) {
				LOG("[-] This processor does not support VMX Extensions.\n");
				LOG_ERROR();
				return false;
			}

			KeLowerIrql(irql);
			KeRevertToUserGroupAffinityThread(&old_affinity);
		}

		return true;
	}

	auto vmx_check_bios_lock() -> bool {
		PAGED_CODE();

		PROCESSOR_NUMBER processor_number;
		GROUP_AFFINITY affinity, old_affinity;

		for (unsigned iter = 0; iter < g_num_processors; iter++) {
			KeGetProcessorNumberFromIndex(iter, &processor_number);

			RtlSecureZeroMemory(&affinity, sizeof(GROUP_AFFINITY));
			affinity.Group = processor_number.Group;
			affinity.Mask = static_cast<KAFFINITY>(1) << processor_number.Number;
			KeSetSystemGroupAffinityThread(&affinity, &old_affinity);

			auto irql = KeRaiseIrqlToDpcLevel();

			ia32_feature_control_register feature_control_msr{};
			feature_control_msr.flags = __readmsr(IA32_FEATURE_CONTROL);

			if (feature_control_msr.lock_bit == 0) {
				// Enable it
				feature_control_msr.lock_bit = 1;
				feature_control_msr.enable_vmx_outside_smx = 1;

				__writemsr(IA32_FEATURE_CONTROL, feature_control_msr.flags);
			}

			if (feature_control_msr.enable_vmx_outside_smx == 0) {
				LOG("[-] EnableVmxOutsideSmx bit not set.\n");
				LOG_ERROR();
				return FALSE;
			}

			KeLowerIrql(irql);
			KeRevertToUserGroupAffinityThread(&old_affinity);
		}

		return true;
	}

	auto vmx_enable_cr4() -> void {
		PAGED_CODE();

		PROCESSOR_NUMBER processor_number;
		GROUP_AFFINITY affinity, old_affinity;

		for (unsigned iter = 0; iter < g_num_processors; iter++) {
			KeGetProcessorNumberFromIndex(iter, &processor_number);

			RtlSecureZeroMemory(&affinity, sizeof(GROUP_AFFINITY));
			affinity.Group = processor_number.Group;
			affinity.Mask = static_cast<KAFFINITY>(1) << processor_number.Number;
			KeSetSystemGroupAffinityThread(&affinity, &old_affinity);

			auto irql = KeRaiseIrqlToDpcLevel();

			PAGED_CODE();

			cr4 _cr4;

			_cr4.flags = __readcr4();
			_cr4.vmx_enable = 1;

			__writecr4(_cr4.flags);

			KeLowerIrql(irql);
			KeRevertToUserGroupAffinityThread(&old_affinity);
		}

		return;
	}

	auto vmx_allocate_vmxon_region(uchar processor_number) -> bool {
		PAGED_CODE();

		if (!vmm_context) {
			LOG("[-] Unspecified VMM context for processor %x\n", processor_number);
			LOG_ERROR();
			return false;
		}

		PHYSICAL_ADDRESS phys_addr;
		phys_addr.QuadPart = static_cast<uint64_t>(~0);

		ia32_vmx_basic_register vmx_basic{};
		vmx_basic.flags = __readmsr(IA32_VMX_BASIC);

		void* vmxon = MmAllocateContiguousMemory(vmx_basic.vmcs_size_in_bytes, phys_addr);
		if (!vmxon) {
			LOG("[-] Allocating vmxon failed for processor (%x).\n", processor_number);
			LOG_ERROR();
			return FALSE;
		}

		RtlSecureZeroMemory(vmxon, vmx_basic.vmcs_size_in_bytes);
		//LOG("[*] Allocated vmxon for processor (%x) at %llx with size %llx\n", processor_number, reinterpret_cast<uint64_t>(vmxon), vmx_basic.vmcs_size_in_bytes);

		*reinterpret_cast<uint64_t*>(vmxon) = vmx_basic.vmcs_revision_id;

		vmm_context[processor_number].vmxon_region_virt_addr = reinterpret_cast<uint64_t>(vmxon);
		vmm_context[processor_number].vmxon_region_phys_addr = virtual_to_physical_address(vmxon);

		//
		// Execute VMXON
		//
		auto ret = __vmx_on(&vmm_context[processor_number].vmxon_region_phys_addr);
		if (ret > 0) {
			LOG("[-] Failed vmxon with error code %x\n", ret);
			LOG_ERROR();
			return false;
		}

		//LOG("[*] vmxon initialized on logical processor (%x)\n", processor_number);
		return true;
	}

	auto vmx_allocate_vmcs_region(uchar processor_number) -> bool {
		PAGED_CODE();

		if (!vmm_context) {
			LOG("[-] Unspecified VMM context for processor %x\n", processor_number);
			LOG_ERROR();
			return false;
		}

		PHYSICAL_ADDRESS phys_addr;
		phys_addr.QuadPart = static_cast<uint64_t>(~0);

		ia32_vmx_basic_register vmx_basic{};
		vmx_basic.flags = __readmsr(IA32_VMX_BASIC);

		void* vmcs = MmAllocateContiguousMemory(vmx_basic.vmcs_size_in_bytes, phys_addr);
		if (!vmcs) {
			LOG("[-] Allocating vmcs failed for processor (%x).\n", processor_number);
			LOG_ERROR();
			return false;
		}

		RtlSecureZeroMemory(vmcs, vmx_basic.vmcs_size_in_bytes);
		//LOG("[*] Allocated vmcs for processor (%x) at %llx with size %llx\n", processor_number, reinterpret_cast<uint64_t>(vmcs), vmx_basic.vmcs_size_in_bytes);

		*reinterpret_cast<uint64_t*>(vmcs) = vmx_basic.vmcs_revision_id;

		vmm_context[processor_number].vmcs_region_virt_addr = reinterpret_cast<UINT64>(vmcs);
		vmm_context[processor_number].vmcs_region_phys_addr = static_cast<uint64_t>(virtual_to_physical_address(vmcs));

		//
		// Load current VMCS and make it active
		//
		auto ret = __vmx_vmptrld(&vmm_context[processor_number].vmcs_region_phys_addr);
		if (ret > 0) {
			LOG("[-] Failed vmcs with error code %x\n", ret);
			LOG_ERROR();
			return FALSE;
		}

		//LOG("[*] vmcs loaded on logical processor (%x)\n", processor_number);
		return true;
	}

	auto vmx_allocate_vmexit_stack(uchar processor_number) -> bool {
		PAGED_CODE();

		PHYSICAL_ADDRESS phys_addr;
		phys_addr.QuadPart = static_cast<uint64_t>(~0);

		void* vmexitStack = MmAllocateContiguousMemory(HOST_STACK_SIZE, phys_addr);
		if (!vmexitStack) {
			LOG("[-] Failure allocating memory for VM EXIT Handler for processor (%x).\n", processor_number);
			LOG_ERROR();
			return false;
		}
		RtlSecureZeroMemory(vmexitStack, HOST_STACK_SIZE);

		vmm_context[processor_number].host_stack = reinterpret_cast<UINT64>(vmexitStack);
		//LOG("[*] vmm_context[%x].HostStack : %llx\n", processor_number, vmm_context[processor_number].host_stack);

		return true;
	}

	auto vmx_allocate_io_bitmap_stack(uchar processor_number) -> bool {
		PAGED_CODE();

		PHYSICAL_ADDRESS phys_addr;
		phys_addr.QuadPart = static_cast<ULONGLONG>(~0);

		void* bitmap = MmAllocateContiguousMemory(PAGE_SIZE, phys_addr);
		if (!bitmap) {
			LOG("[-] Failure allocating memory for IO Bitmap A on processor (%x).\n", processor_number);
			LOG_ERROR();
			return false;
		}
		RtlSecureZeroMemory(bitmap, PAGE_SIZE);

		vmm_context[processor_number].io_bitmap_a_virt_addr = reinterpret_cast<uint64_t>(bitmap);
		vmm_context[processor_number].io_bitmap_a_phys_addr = static_cast<uint64_t>(virtual_to_physical_address(bitmap));

		phys_addr.QuadPart = static_cast<ULONGLONG>(~0);
		bitmap = MmAllocateContiguousMemory(PAGE_SIZE, phys_addr);
		if (!bitmap) {
			LOG("[-] Failure allocating memory for IO Bitmap B on processor (%x).\n", processor_number);
			LOG_ERROR();
			return false;
		}
		RtlSecureZeroMemory(bitmap, PAGE_SIZE);

		vmm_context[processor_number].io_bitmap_b_virt_addr = reinterpret_cast<uint64_t>(bitmap);
		vmm_context[processor_number].io_bitmap_b_phys_addr = static_cast<uint64_t>(virtual_to_physical_address(bitmap));

		//LOG("[*] vmm_context[%x].io_bitmap_a_virt_addr : %llx\n", processor_number, vmm_context[processor_number].io_bitmap_a_virt_addr);
		//LOG("[*] vmm_context[%x].io_bitmap_b_virt_addr : %llx\n", processor_number, vmm_context[processor_number].io_bitmap_b_virt_addr);

		//
		// We want to vmexit on every io and msr access
		//memset(vmm_context[processor_number].io_bitmap_a_virt_addr, 0xff, PAGE_SIZE);
		//memset(vmm_context[processor_number].io_bitmap_b_virt_addr, 0xff, PAGE_SIZE);

		return true;
	}

	auto vmx_allocate_msr_bitmap(uchar processor_number) -> bool {
		PAGED_CODE();

		PHYSICAL_ADDRESS phys_addr;
		phys_addr.QuadPart = static_cast<ULONGLONG>(~0);

		void* bitmap = MmAllocateContiguousMemory(PAGE_SIZE, phys_addr);
		if (!bitmap) {
			LOG("[-] Failure allocating memory for MSR Bitmap for processor (%x).\n", processor_number);
			LOG_ERROR();
			return false;
		}
		RtlSecureZeroMemory(bitmap, PAGE_SIZE);

		vmm_context[processor_number].msr_bitmap_virt_addr = reinterpret_cast<uint64_t>(bitmap);
		vmm_context[processor_number].msr_bitmap_phys_addr = static_cast<uint64_t>(virtual_to_physical_address(bitmap));

		return true;
	}

	auto vmx_handle_vmcall(unsigned __int64 vmcall_number, unsigned __int64 param1, unsigned __int64 param2, unsigned __int64 param3) -> unsigned __int64 {
		UNREFERENCED_PARAMETER(param1);
		UNREFERENCED_PARAMETER(param2);
		UNREFERENCED_PARAMETER(param3);
		
		LOG("[*] Executing vmcall with number : %x\n", vmcall_number);
		unsigned __int64 vmcall_status = STATUS_SUCCESS;

		switch (vmcall_number) {
		case vmcall_numbers::vmx_test_vmcall:
			break;

		case vmcall_numbers::vmx_vmoff:
			break;

		case vmcall_numbers::vmx_hook_page:
			break;

		case vmcall_numbers::vmx_invept_global_context:
			break;

		case vmcall_numbers::vmx_invept_single_context:
			break;

		default:
			__debugbreak();
			vmcall_status = static_cast<unsigned __int64>(STATUS_UNSUCCESSFUL);
		}

		return vmcall_status;
	}
}
