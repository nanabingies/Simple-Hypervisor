#include "stdafx.h"
_vmm_context* vmm_context;

namespace vmx {
	auto vmx_is_vmx_available() -> bool {
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
		//using ept::check_ept_support;
		//if (!check_ept_support())	return false;


		//LOG("[*] Initial checks completed.\n");

		return TRUE;
	}

	auto vmx_is_vmx_support() -> bool {
		PROCESSOR_NUMBER processor_number;
		GROUP_AFFINITY affinity, old_affinity;

		for (unsigned iter = 0; iter < g_num_processors; iter++) {
			KeGetProcessorNumberFromIndex(iter, &processor_number);

			RtlSecureZeroMemory(&affinity, sizeof(GROUP_AFFINITY));
			affinity.Group = processor_number.Group;
			affinity.Mask = static_cast<KAFFINITY>(1) << processor_number.Number;
			KeSetSystemGroupAffinityThread(&affinity, &old_affinity);

			auto irql = KeRaiseIrqlToDpcLevel();

			//LOG("[*] Checking processor (%x) VMX support.....\n", processor_number.Number);
			cpuid_eax_01 args{};
			__cpuid(reinterpret_cast<int32_t*>(&args), 1);

			if (args.cpuid_feature_information_ecx.virtual_machine_extensions == 0) {
				LOG("[-] This processor does not support VMX Extensions.\n");
				LOG_ERROR(__FILE__, __LINE__);
				return false;
			}

			KeLowerIrql(irql);
			KeRevertToUserGroupAffinityThread(&old_affinity);
		}

		return true;
	}

	auto vmx_check_bios_lock() -> bool {
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
				LOG_ERROR(__FILE__, __LINE__);
				return FALSE;
			}

			KeLowerIrql(irql);
			KeRevertToUserGroupAffinityThread(&old_affinity);
		}

		return true;
	}

	auto vmx_enable_cr4() -> void {
		PROCESSOR_NUMBER processor_number;
		GROUP_AFFINITY affinity, old_affinity;

		for (unsigned iter = 0; iter < g_num_processors; iter++) {
			KeGetProcessorNumberFromIndex(iter, &processor_number);

			RtlSecureZeroMemory(&affinity, sizeof(GROUP_AFFINITY));
			affinity.Group = processor_number.Group;
			affinity.Mask = static_cast<KAFFINITY>(1) << processor_number.Number;
			KeSetSystemGroupAffinityThread(&affinity, &old_affinity);

			auto irql = KeRaiseIrqlToDpcLevel();

			cr4 _cr4;

			_cr4.flags = __readcr4();
			_cr4.vmx_enable = 1;

			__writecr4(_cr4.flags);

			KeLowerIrql(irql);
			KeRevertToUserGroupAffinityThread(&old_affinity);
		}

		return;
	}

	auto create_vcpus() -> bool {
		g_vmx_ctx.vcpu_count = KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);
		LOG("[*] vcpu count : %x\n", g_vmx_ctx.vcpu_count);

		for (unsigned iter = 0; iter < g_vmx_ctx.vcpu_count; iter++) {
			if (!vmx_allocate_vmxon_region(&g_vmx_ctx.vcpus[iter]))	return false;
			if (!vmx_allocate_vmcs_region(&g_vmx_ctx.vcpus[iter]))	return false;
		}

		return true;
	}

	auto vmx_allocate_vmxon_region(struct _vcpu_ctx* vcpu_ctx) -> bool {
		if (!vcpu_ctx) {
			LOG("[!] Unspecified VMM context for processor %x\n", KeGetCurrentProcessorNumber());
			LOG_ERROR(__FILE__, __LINE__);
			return false;
		}

		PHYSICAL_ADDRESS phys_addr;
		phys_addr.QuadPart = static_cast<uint64_t>(~0);

		ia32_vmx_basic_register vmx_basic{};
		vmx_basic.flags = __readmsr(IA32_VMX_BASIC);

		vcpu_ctx->vmxon_phys = virtual_to_physical_address(&vcpu_ctx->vmxon);
		vcpu_ctx->vmxon.header.bits.revision_identifier = vmx_basic.vmcs_revision_id;
		
		cr_fixed_t cr_fixed{};
		cr0 _cr0{};
		cr4 _cr4{};
		cr_fixed.all = __readmsr(IA32_VMX_CR0_FIXED0);
		_cr0.flags = __readcr0();
		_cr0.flags |= cr_fixed.split.low;
		cr_fixed.all = __readmsr(IA32_VMX_CR0_FIXED1);
		_cr0.flags &= cr_fixed.split.low;
		__writecr0(_cr0.flags);

		cr_fixed.all = __readmsr(IA32_VMX_CR4_FIXED0);
		_cr4.flags = __readcr4();
		_cr4.flags |= cr_fixed.split.low;
		cr_fixed.all = __readmsr(IA32_VMX_CR4_FIXED1);
		_cr4.flags &= cr_fixed.split.low;
		__writecr4(_cr4.flags);

		auto rets = __vmx_on(static_cast<unsigned long long*>(&vcpu_ctx->vmxon_phys));
		if (rets != 0) {	// Failure
			LOG("[!] Failed to activate virtual machine extensions (VMX) operation on processor (%x)\n", KeGetCurrentProcessorNumber());
			LOG_ERROR(__FILE__, __LINE__);
			return false;
		}

		return true;
	}

	auto vmx_allocate_vmcs_region(struct _vcpu_ctx* vcpu_ctx) -> bool {
		if (!vcpu_ctx) {
			LOG("[!] Unspecified VMM context for processor %x\n", KeGetCurrentProcessorNumber());
			LOG_ERROR(__FILE__, __LINE__);
			return false;
		}

		PHYSICAL_ADDRESS phys_addr;
		phys_addr.QuadPart = static_cast<uint64_t>(~0);

		ia32_vmx_basic_register vmx_basic{};
		vmx_basic.flags = __readmsr(IA32_VMX_BASIC);

		vcpu_ctx->vmcs_phys = virtual_to_physical_address(&vcpu_ctx->vmcs);
		vcpu_ctx->vmcs.header.bits.revision_identifier = vmx_basic.vmcs_revision_id;

		auto rets = __vmx_vmptrld(static_cast<unsigned long long*>(&vcpu_ctx->vmcs_phys));
		if (rets != 0) {	// Failure
			LOG("[!] Failed to activate virtual machine extensions (VMX) operation on processor (%x)\n", KeGetCurrentProcessorNumber());
			LOG_ERROR(__FILE__, __LINE__);
			return false;
		}
		
		return true;
	}

	auto vmx_allocate_vmexit_stack(uchar processor_number) -> bool {
		PHYSICAL_ADDRESS phys_addr;
		phys_addr.QuadPart = static_cast<uint64_t>(~0);

		void* vmexitStack = MmAllocateContiguousMemory(HOST_STACK_SIZE, phys_addr);
		if (!vmexitStack) {
			LOG("[-] Failure allocating memory for VM EXIT Handler for processor (%x).\n", processor_number);
			LOG_ERROR(__FILE__, __LINE__);
			return false;
		}
		RtlSecureZeroMemory(vmexitStack, HOST_STACK_SIZE);

		vmm_context[processor_number].host_stack = reinterpret_cast<UINT64>(vmexitStack);
		//LOG("[*] vmm_context[%x].HostStack : %llx\n", processor_number, vmm_context[processor_number].host_stack);

		return true;
	}

	auto vmx_allocate_io_bitmap_stack(uchar processor_number) -> bool {
		PHYSICAL_ADDRESS phys_addr;
		phys_addr.QuadPart = static_cast<ULONGLONG>(~0);

		void* bitmap = MmAllocateContiguousMemory(PAGE_SIZE, phys_addr);
		if (!bitmap) {
			LOG("[-] Failure allocating memory for IO Bitmap A on processor (%x).\n", processor_number);
			LOG_ERROR(__FILE__, __LINE__);
			return false;
		}
		RtlSecureZeroMemory(bitmap, PAGE_SIZE);

		vmm_context[processor_number].io_bitmap_a_virt_addr = reinterpret_cast<uint64_t>(bitmap);
		vmm_context[processor_number].io_bitmap_a_phys_addr = static_cast<uint64_t>(virtual_to_physical_address(bitmap));

		phys_addr.QuadPart = static_cast<ULONGLONG>(~0);
		bitmap = MmAllocateContiguousMemory(PAGE_SIZE, phys_addr);
		if (!bitmap) {
			LOG("[-] Failure allocating memory for IO Bitmap B on processor (%x).\n", processor_number);
			LOG_ERROR(__FILE__, __LINE__);
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
		PHYSICAL_ADDRESS phys_addr;
		phys_addr.QuadPart = static_cast<ULONGLONG>(~0);

		void* bitmap = MmAllocateContiguousMemory(PAGE_SIZE, phys_addr);
		if (!bitmap) {
			LOG("[-] Failure allocating memory for MSR Bitmap for processor (%x).\n", processor_number);
			LOG_ERROR(__FILE__, __LINE__);
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
		
		//LOG("[*] Executing vmcall with number : %x\n", vmcall_number);
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
