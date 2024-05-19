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

			if (init_vmcs(g_vmm_context->vcpu_table[iter]) == false)
				return false;
		}

		KeGenericCallDpc(dpc_broadcast_initialize_guest, 0);
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

	auto init_vmcs(struct __vcpu*& vcpu) -> bool {
		ia32_vmx_basic_register vmx_basic = { 0 };
		PHYSICAL_ADDRESS physical_max;

		vmx_basic.flags = __readmsr(IA32_VMX_BASIC);

		physical_max.QuadPart = ~0ULL;
		vcpu->vmcs = reinterpret_cast<__vmcs*>(ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, VMM_POOL_TAG));
		if (vcpu->vmcs == nullptr) {
			LOG("[!] vmcs structure for processor (%x) could not be allocated");
			LOG_ERROR(__FILE__, __LINE__);
			return false;
		}

		vcpu->vmcs_physical = MmGetPhysicalAddress(vcpu->vmcs).QuadPart;
		if (vcpu->vmcs_physical == 0)	return false;

		RtlSecureZeroMemory(vcpu->vmcs, PAGE_SIZE);
		vcpu->vmcs->header.revision_identifier = vmx_basic.vmcs_revision_id;

		// Indicates if it's shadow vmcs or not
		vcpu->vmcs->header.shadow_vmcs_indicator = 0;

		return true;
	}

	auto dpc_broadcast_initialize_guest(KDPC* Dpc, void* DeferredContext, void* SystemArgument1, void* SystemArgument2) -> void {
		UNREFERENCED_PARAMETER(DeferredContext);
		UNREFERENCED_PARAMETER(Dpc);

		asm_save_vmm_state();

		// Wait for all DPCs to synchronize at this point
		KeSignalCallDpcSynchronize(SystemArgument2);

		// Mark the DPC as being complete
		KeSignalCallDpcDone(SystemArgument1);
	}

	auto initialize_vmm(void* guest_rsp) -> void {
		auto current_procesor = KeGetCurrentProcessorNumberEx(nullptr);
		auto current_vcpu = g_vmm_context->vcpu_table[current_procesor];
		
		if (__vmx_on(&current_vcpu->vmxon_physical)) {
			LOG("[!] Failed to put vcpu %d into VMX operation.\n", current_procesor);
			LOG_ERROR(__FILE__, __LINE__);
			return;
		}

		current_vcpu->vcpu_status.vmx_on = true;
		hv_setup_vmcs(current_vcpu, guest_rsp);
		current_vcpu->vcpu_status.vmm_launched = true;

		__vmx_vmlaunch();

		// We should never get here
		__debugbreak();
		size_t error_code = 0;
		__vmx_vmread(VMCS_VM_INSTRUCTION_ERROR, &error_code);
		LOG("[!] Failed to launch vmm on processor (%x) with error code : %x\n", current_procesor, error_code);
		current_vcpu->vcpu_status.vmx_on = false;
		current_vcpu->vcpu_status.vmm_launched = false;
	}

	auto get_system_dirbase() -> unsigned __int64 {
		__debugbreak();
		UNICODE_STRING us_string{};
		RtlInitUnicodeString(&us_string, L"PsInitialSystemProcess");
		auto initial_process = MmGetSystemRoutineAddress(&us_string);
		DbgPrint("[*] PsInitialSystemProcess : %p\n", initial_process);
		auto dir_base = ((_KPROCESS*)initial_process)->DirectoryTableBase;
		DbgPrint("[*] dirbase : %llx\n", dir_base);
		return ((_KPROCESS*)PsInitialSystemProcess)->DirectoryTableBase;
	}

}
