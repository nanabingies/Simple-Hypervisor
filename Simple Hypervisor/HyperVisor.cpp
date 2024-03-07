#include "stdafx.h"
#pragma warning(disable: 4996)

namespace hv {
	auto virtualize_all_processors() -> bool {
		using vmx::vmx_allocate_vmxon_region;
		using vmx::vmx_allocate_vmcs_region;
		using vmx::vmx_allocate_vmexit_stack;
		using vmx::vmx_allocate_io_bitmap_stack;
		using vmx::vmx_allocate_msr_bitmap;

		PAGED_CODE();

		//
		// This was more of an educational project so only one Logical Processor was chosen and virtualized
		// TODO : Add support for multiple processors
		// Update: Support for multiple processors added
		//

		PROCESSOR_NUMBER processor_number;
		GROUP_AFFINITY affinity, old_affinity;

		//
		// ExAllocatePool2 - Memory is zero initialized unless POOL_FLAG_UNINITIALIZED is specified.
		//
		vmm_context = reinterpret_cast<_vmm_context*>
			(ExAllocatePoolWithTag(NonPagedPool, sizeof(_vmm_context) * g_num_processors, VMM_POOL_TAG));
		if (!vmm_context) {
			LOG("[-] Failed to allocate memory for vmm_context\n");
			LOG_ERROR();
			return false;
		}

		for (unsigned iter = 0; iter < g_num_processors; iter++) {
			KeGetProcessorNumberFromIndex(iter, &processor_number);

			RtlSecureZeroMemory(&affinity, sizeof(GROUP_AFFINITY));
			affinity.Group = processor_number.Group;
			affinity.Mask = (KAFFINITY)1 << processor_number.Number;
			KeSetSystemGroupAffinityThread(&affinity, &old_affinity);
			auto irql = KeRaiseIrqlToDpcLevel();

			//
			// Allocate Memory for VMXON & VMCS regions and initialize
			//
			if (!vmx_allocate_vmxon_region(processor_number.Number))		return false;
			if (!vmx_allocate_vmcs_region(processor_number.Number))		return false;

			//
			// Allocate space for VM EXIT Handler
			//
			if (!vmx_allocate_vmexit_stack(processor_number.Number))		return false;

			//
			// Allocate memory for IO Bitmap
			//
			if (!vmx_allocate_io_bitmap_stack(processor_number.Number))			return false;

			//
			// Future: Add MSR Bitmap support
			// Update: Added MSR Bitmap support
			//
			if (!vmx_allocate_msr_bitmap(processor_number.Number))			return false;

			//
			// Setup EPT support for that processor
			//
			//if (!initializeEpt(processor_number.Number))				return false;

			KeLowerIrql(irql);
			KeRevertToUserGroupAffinityThread(&old_affinity);
		}

		return true;
	}

	auto devirtualize_all_processors() -> void {
		PROCESSOR_NUMBER processor_number;
		GROUP_AFFINITY affinity, old_affinity;

		for (unsigned iter = 0; iter < g_num_processors; iter++) {
			KeGetProcessorNumberFromIndex(iter, &processor_number);

			RtlSecureZeroMemory(&affinity, sizeof(GROUP_AFFINITY));
			affinity.Group = processor_number.Group;
			affinity.Mask = (KAFFINITY)1 << processor_number.Number;
			KeSetSystemGroupAffinityThread(&affinity, &old_affinity);

			auto irql = KeRaiseIrqlToDpcLevel();

			__vmx_vmclear(&vmm_context[processor_number.Number].vmcs_region_phys_addr);
			__vmx_off();

			if (vmm_context[processor_number.Number].vmcs_region_virt_addr)
				MmFreeContiguousMemory(reinterpret_cast<void*>(vmm_context[processor_number.Number].vmcs_region_virt_addr));

			if (vmm_context[processor_number.Number].vmxon_region_virt_addr)
				MmFreeContiguousMemory(reinterpret_cast<void*>(vmm_context[processor_number.Number].vmxon_region_virt_addr));

			if (vmm_context[processor_number.Number].host_stack)
				MmFreeContiguousMemory(reinterpret_cast<void*>(vmm_context[processor_number.Number].host_stack));

			if (vmm_context[processor_number.Number].io_bitmap_a_virt_addr)
				MmFreeContiguousMemory(reinterpret_cast<void*>(vmm_context[processor_number.Number].io_bitmap_a_virt_addr));

			if (vmm_context[processor_number.Number].io_bitmap_b_virt_addr)
				MmFreeContiguousMemory(reinterpret_cast<void*>(vmm_context[processor_number.Number].io_bitmap_b_virt_addr));

			if (vmm_context[processor_number.Number].msr_bitmap_virt_addr)
				MmFreeContiguousMemory(reinterpret_cast<void*>(vmm_context[processor_number.Number].msr_bitmap_virt_addr));

			/*if (vmm_context[processor_number.Number].EptState) {
				if (vmm_context[processor_number.Number].EptState->EptPageTable)
					ExFreePoolWithTag(vmm_context[processor_number.Number].EptState->EptPageTable, VMM_POOL);

				if (vmm_context[processor_number.Number].EptState->EptPtr)
					ExFreePoolWithTag(vmm_context[processor_number.Number].EptState->EptPtr, VMM_POOL);

				ExFreePoolWithTag(vmm_context[processor_number.Number].EptState, VMM_POOL);
			}*/

			KeLowerIrql(irql);
			KeRevertToUserGroupAffinityThread(&old_affinity);
		}

		ExFreePoolWithTag(vmm_context, VMM_POOL_TAG);

		return;
	}

	auto launch_vm(unsigned __int64) -> unsigned __int64 {
		auto processor_number = KeGetCurrentProcessorNumber();

		//
		// Set VMCS state to inactive
		//
		unsigned char ret = __vmx_vmclear(&vmm_context[processor_number].vmcs_region_phys_addr);
		if (ret > 0) {
			LOG("[-] VMCLEAR operation failed.\n");
			LOG_ERROR();
			return 0;
		}
		LOG("[*] VMCS for processor (%x) set to inactive.\n", processor_number);

		//
		//  Make VMCS the current and active on that processor
		//
		ret = __vmx_vmptrld(&vmm_context[processor_number].vmcs_region_phys_addr);
		if (ret > 0) {
			LOG("[-] VMPTRLD operation failed.\n");
			LOG_ERROR();
			return 0;
		}
		LOG("[*] VMCS is current and active on processor %x\n", processor_number);

		//
		// Setup VMCS structure fields for that logical processor
		//
		if (asm_setup_vmcs(processor_number) != VM_ERROR_OK) {
			DbgPrint("[-] Failure setting Virtual Machine VMCS.\n");

			size_t ErrorCode = 0;
			__vmx_vmread(VMCS_VM_INSTRUCTION_ERROR, &ErrorCode);
			DbgPrint("[-] Exiting with error code : %llx\n", ErrorCode);
			LOG_ERROR();
			return 0;
		}
		DbgPrint("[*] VMCS setup on processor %x done\n", processor_number);
		vmexit::vmexit_handler(nullptr);


		//
		// Launch VM into Outer Space :)
		//
		//__vmx_vmlaunch();

		// We should never get here
		//DbgBreakPoint();
		//LOG("[-] Failure launching Virtual Machine.\n");

		//size_t ErrorCode = 0;
		//__vmx_vmread(VMCS_VM_INSTRUCTION_ERROR, &ErrorCode);
		//LOG("[-] Exiting with error code : %llx\n", ErrorCode);

		return 0;
	}

	auto inline terminate_vm(uchar processor_number) -> void {

		//
		// Set VMCS state to inactive
		//
		__vmx_vmclear(&vmm_context[processor_number].vmcs_region_phys_addr);

		return;
	}

	auto resume_vm(uchar processor_number) -> void {
		UNREFERENCED_PARAMETER(processor_number);

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

}
