#include "stdafx.h"

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

		for (unsigned iter = 0; iter < 4 /*g_num_processors*/; iter++) {
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

		for (unsigned iter = 0; iter < 4 /*g_num_processors*/; iter++) {
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

		for (unsigned iter = 0; iter < 4 /*g_num_processors*/; iter++) {
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
			if (!vmx_allocate_io_bitmap_stack(&g_vmx_ctx.vcpus[iter]))	return false;
			if (!vmx_allocate_msr_bitmap(&g_vmx_ctx.vcpus[iter]))	return false;
		}

		return true;
	}

	auto vmx_allocate_vmxon_region(struct _vcpu_ctx* vcpu_ctx) -> bool {
		if (!vcpu_ctx) {
			LOG("[!] Unspecified VMM context for processor %x\n", KeGetCurrentProcessorNumber());
			LOG_ERROR(__FILE__, __LINE__);
			return false;
		}

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

	auto vmx_allocate_io_bitmap_stack(struct _vcpu_ctx* vcpu_ctx) -> bool {
		if (!vcpu_ctx) {
			LOG("[!] Unspecified VMM context for processor %x\n", KeGetCurrentProcessorNumber());
			LOG_ERROR(__FILE__, __LINE__);
			return false;
		}

		vcpu_ctx->io_bitmap_a_phys = virtual_to_physical_address(&vcpu_ctx->io_bitmap_a);
		vcpu_ctx->io_bitmap_b_phys = virtual_to_physical_address(&vcpu_ctx->io_bitmap_b);

		//
		// We want to vmexit on every io and msr access
		memset(vcpu_ctx->io_bitmap_a.data, 0xff, sizeof vcpu_ctx->io_bitmap_a.data);
		memset(vcpu_ctx->io_bitmap_b.data, 0xff, sizeof vcpu_ctx->io_bitmap_b.data);

		return true;
	}

	auto vmx_allocate_msr_bitmap(struct _vcpu_ctx* vcpu_ctx) -> bool {
		if (!vcpu_ctx) {
			LOG("[!] Unspecified VMM context for processor %x\n", KeGetCurrentProcessorNumber());
			LOG_ERROR(__FILE__, __LINE__);
			return false;
		}

		vcpu_ctx->msr_bitmap_phys = virtual_to_physical_address(&vcpu_ctx->msr_bitmap);

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
