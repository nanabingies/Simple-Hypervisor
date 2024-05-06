#include "stdafx.h"
#pragma warning(disable: 4996)

namespace hv {
	auto vmm_init() -> bool {
		using vmx::vmx_allocate_vmm_context;

		if (!vmx_allocate_vmm_context()) {
			LOG("[!] Failed to allocate memory for vmm_context\n");
			LOG_ERROR(__FILE__, __LINE__);
			return false;
		}

		// Initialize vcpu for each logical processor
		for (unsigned iter = 0; iter < g_vmm_context->processor_count; iter++) {
			if (init_vcpu(g_vmm_context->vcpu_table[iter]) == false)
				return false;

			if (init_vmxon(g_vmm_context->vcpu_table[iter]) == false)
				return false;

			//if (init_vmcs(g_vmm_context->vcpu_table[iter]) == false)
			//	return false;
		}

		//KeGenericCallDpc(dpc_broadcast_initialize_guest, 0);
		return true;
	}

	auto init_vcpu(struct __vcpu*& vcpu) -> bool {
		unsigned curr_processor = KeGetCurrentProcessorNumber();
		vcpu = reinterpret_cast<__vcpu*>(ExAllocatePoolWithTag(NonPagedPool, sizeof(__vcpu), VMM_POOL_TAG));
		if (vcpu == nullptr) {
			LOG("[!] Failed to create vcpu for processor (%x)\n", curr_processor);
			LOG_ERROR(__FILE__, __LINE__);
			return false;
		}

		RtlSecureZeroMemory(vcpu, sizeof(__vcpu));

		vcpu->vmm_stack = reinterpret_cast<void*>(ExAllocatePoolWithTag(NonPagedPool, VMM_STACK_SIZE, VMM_POOL_TAG));
		if (vcpu->vmm_stack == nullptr){
			LOG("[!] vcpu stack for processor (%x) could not be allocated\n", curr_processor);
			LOG_ERROR(__FILE__, __LINE__);
			return false;
		}
		RtlSecureZeroMemory(vcpu->vmm_stack, VMM_STACK_SIZE);

		vcpu->vcpu_bitmaps.msr_bitmap = reinterpret_cast<unsigned char*>(ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, VMM_POOL_TAG));
		if (vcpu->vcpu_bitmaps.msr_bitmap == nullptr) {
			LOG("[!] vcpu msr bitmap for processor (%x) could not be allocated\n", curr_processor);
			LOG_ERROR(__FILE__, __LINE__);
			return false;
		}
		RtlSecureZeroMemory(vcpu->vcpu_bitmaps.msr_bitmap, PAGE_SIZE);
		vcpu->vcpu_bitmaps.msr_bitmap_physical = MmGetPhysicalAddress(vcpu->vcpu_bitmaps.msr_bitmap).QuadPart;

		vcpu->vcpu_bitmaps.io_bitmap_a = reinterpret_cast<unsigned char*>(ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, VMM_POOL_TAG));
		if (vcpu->vcpu_bitmaps.io_bitmap_a == nullptr) {
			LOG("[!] vcpu io bitmap a for processor (%x) could not be allocated\n", curr_processor);
			LOG_ERROR(__FILE__, __LINE__);
			return false;
		}
		RtlSecureZeroMemory(vcpu->vcpu_bitmaps.io_bitmap_a, PAGE_SIZE);
		vcpu->vcpu_bitmaps.io_bitmap_a_physical = MmGetPhysicalAddress(vcpu->vcpu_bitmaps.io_bitmap_a).QuadPart;

		vcpu->vcpu_bitmaps.io_bitmap_b = reinterpret_cast<unsigned char*>(ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, VMM_POOL_TAG));
		if (vcpu->vcpu_bitmaps.io_bitmap_b == nullptr) {
			LOG("[!] vcpu io bitmap b for processor (%x) could not be allocated\n", curr_processor);
			LOG_ERROR(__FILE__, __LINE__);
			return false;
		}
		RtlSecureZeroMemory(vcpu->vcpu_bitmaps.io_bitmap_b, PAGE_SIZE);
		vcpu->vcpu_bitmaps.io_bitmap_b_physical = MmGetPhysicalAddress(vcpu->vcpu_bitmaps.io_bitmap_b).QuadPart;

		//
		// Allocate ept state structure
		//
		vcpu->ept_state = reinterpret_cast<__ept_state*>(ExAllocatePoolWithTag(NonPagedPool, sizeof(__ept_state), VMM_POOL_TAG));
		if (vcpu->ept_state == nullptr) {
			LOG("[!] vcpu ept state for processor (%x) could not be allocated\n", curr_processor);
			LOG_ERROR(__FILE__, __LINE__);
			return false;
		}
		RtlSecureZeroMemory(vcpu->ept_state, sizeof(__ept_state));
		InitializeListHead(&vcpu->ept_state->hooked_page_list);

		return true;
	}

	auto init_vmxon(struct __vcpu*& vcpu) -> bool {
		unsigned curr_processor = KeGetCurrentProcessorNumber();

		ia32_vmx_basic_register vmx_basic{};
		vmx_basic.flags = __readmsr(IA32_VMX_BASIC);

		vcpu->vmxon = reinterpret_cast<__vmcs*>(ExAllocatePoolWithTag(NonPagedPool, vmx_basic.vmcs_size_in_bytes, VMM_POOL_TAG));
		if (vcpu->vmxon == nullptr) {
			LOG("[!] vmxon could not be allocated on processor (%x)\n", curr_processor);
			LOG_ERROR(__FILE__, __LINE__);
			return false;
		}

		vcpu->vmxon_physical = MmGetPhysicalAddress(vcpu->vmxon).QuadPart;
		if (vcpu->vmxon_physical == 0) {
			LOG("[!] Could not get vmxon physical address on processor (%x)\n", curr_processor);
			LOG_ERROR(__FILE__, __LINE__);
			return false;
		}

		RtlSecureZeroMemory(vcpu->vmxon, vmx_basic.vmcs_size_in_bytes);
		vcpu->vmxon->header.all = vmx_basic.vmcs_revision_id;
		vcpu->vmxon->header.shadow_vmcs_indicator = 0;

		return true;
	}

	auto launch_vm(ULONG_PTR arg) -> ULONG_PTR {
		ulong processor_number = KeGetCurrentProcessorNumber();

		//
		// Set VMCS state to inactive
		//
		unsigned char ret = __vmx_vmclear(&vmm_context[processor_number].vmcs_region_phys_addr);
		if (ret > 0) {
			LOG("[-] VMCLEAR operation failed.\n");
			LOG_ERROR(__FILE__, __LINE__);
			return arg;
		}

		//
		//  Make VMCS the current and active on that processor
		//
		ret = __vmx_vmptrld(&vmm_context[processor_number].vmcs_region_phys_addr);
		if (ret > 0) {
			LOG("[-] VMPTRLD operation failed.\n");
			LOG_ERROR(__FILE__, __LINE__);
			return arg;
		}

		//
		//	read cr3 and save for guest
		//
		cr3_val = __readcr3();

		//
		// Setup VMCS structure fields for that logical processor
		//
		if (asm_setup_vmcs(processor_number) != VM_ERROR_OK) {
			LOG("[-] Failure setting Virtual Machine VMCS for processor %x.\n", processor_number);

			size_t error_code = 0;
			__vmx_vmread(VMCS_VM_INSTRUCTION_ERROR, &error_code);
			LOG("[-] Exiting with error code : %llx\n", error_code);
			return arg;
		}
		//LOG("[*] VMCS setup on processor %x done\n", processor_number);

		//
		// Launch VM into Outer Space :)
		//
		__vmx_vmlaunch();

		// We should never get here
		DbgBreakPoint();
		LOG("[-] Failure launching Virtual Machine.\n");

		size_t error_code = 0;
		__vmx_vmread(VMCS_VM_INSTRUCTION_ERROR, &error_code);
		LOG("[-] Exiting with error code : %llx\n", error_code);

		return arg;
	}

	auto dpc_broadcast_initialize_guest(KDPC* Dpc, void* DeferredContext, void* SystemArgument1, void* SystemArgument2) -> void {
		UNREFERENCED_PARAMETER(DeferredContext);
		UNREFERENCED_PARAMETER(Dpc);

		launch_vm(0);

		// Wait for all DPCs to synchronize at this point
		KeSignalCallDpcSynchronize(SystemArgument2);

		// Mark the DPC as being complete
		KeSignalCallDpcDone(SystemArgument1);
	}

	auto launch_all_vmms() -> void {
		
		KeIpiGenericCall((PKIPI_BROADCAST_WORKER)launch_vm, 0);
		//KeGenericCallDpc(dpc_broadcast_initialize_guest, 0);

		return;
	}

	auto inline terminate_vm(uchar processor_number) -> void {

		//
		// Set VMCS state to inactive
		//
		__vmx_vmclear(&vmm_context[processor_number].vmcs_region_phys_addr);

		return;
	}

	auto resume_vm() -> void {
		//UNREFERENCED_PARAMETER(processor_number);

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
