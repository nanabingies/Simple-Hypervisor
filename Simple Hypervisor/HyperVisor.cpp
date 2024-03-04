#include "vmx.hpp"
#pragma warning(disable: 4996)

namespace hv {
	auto virtualizeAllProcessors() -> bool {
		using vmx::vmxAllocateVmxonRegion;
		using vmx::vmxAllocateVmcsRegion;
		using vmx::vmxAllocateVmExitStack;
		using vmx::vmxAllocateIoBitmapStack;
		using vmx::vmxAllocateMsrBitmap;

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
			if (!vmxAllocateVmxonRegion(processor_number.Number))		return false;
			if (!vmxAllocateVmcsRegion(processor_number.Number))		return false;

			//
			// Allocate space for VM EXIT Handler
			//
			if (!vmxAllocateVmExitStack(processor_number.Number))		return false;

			//
			// Allocate memory for IO Bitmap
			//
			if (!vmxAllocateIoBitmapStack(processor_number.Number))			return false;

			//
			// Future: Add MSR Bitmap support
			// Update: Added MSR Bitmap support
			//
			if (!vmxAllocateMsrBitmap(processor_number.Number))			return false;

			//
			// Setup EPT support for that processor
			//
			//if (!initializeEpt(processor_number.Number))				return false;

			KeLowerIrql(irql);
			KeRevertToUserGroupAffinityThread(&old_affinity);
		}

		return true;
	}

	auto devirtualizeAllProcessors() -> void {
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

	auto launchVM(unsigned __int64) -> unsigned __int64 {

		return 0;
	}

}
