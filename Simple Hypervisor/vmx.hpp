#pragma once
#include <basetsd.h>
using uchar = unsigned char;

enum vmcall_numbers : unsigned long { 
	vmx_test_vmcall = 0ul,
	vmx_vmoff,
	vmx_hook_page,
	vmx_invept_global_context,
	vmx_invept_single_context,
};

union __cr_fixed
{
	unsigned __int64 all;
	struct
	{
		unsigned long low;
		long high;
	} split;
	struct
	{
		unsigned long low;
		long high;
	} u;
};

namespace vmx {
	auto vmx_is_vmx_available() -> bool;
	auto vmx_is_vmx_support() -> bool;
	auto vmx_check_bios_lock() -> bool;
	auto vmx_enable_cr4() -> void;
	auto disable_vmx() -> void;

	auto vmx_allocate_vmm_context() -> bool;
	auto vmx_free_vmm_context() -> void;
	auto vmx_allocate_vmxon_region(uchar) -> bool;
	auto vmx_allocate_vmcs_region(uchar) -> bool;
	auto vmx_allocate_vmexit_stack(uchar) -> bool;
	auto vmx_allocate_io_bitmap_stack(uchar) -> bool;
	auto vmx_allocate_msr_bitmap(uchar) -> bool;

	auto adjust_control_registers() -> void;

	auto vmx_handle_vmcall(unsigned __int64, unsigned __int64, unsigned __int64, unsigned __int64) -> unsigned __int64;
}

namespace hv {
	auto vmm_init() -> bool;
	auto init_vcpu(struct __vcpu*&) -> bool;
	auto init_vmxon(struct __vcpu*&) -> bool;
	auto init_vmcs(struct __vcpu*&) -> bool;

	extern "C" auto inline asm_save_vmm_state() -> void;
	auto dpc_broadcast_initialize_guest(struct _KDPC*, void*, void*, void*) -> void;

	auto initialize_vmm(void*) -> void;

	auto get_system_dirbase() -> unsigned __int64;

}

namespace vmexit {
	auto vmexit_handler(void*) -> void;

	auto handle_task_switch(void*) -> void;
	auto handle_wrmsr(void*) -> void;
	auto handle_rdmsr(void*) -> void;
	auto handle_execute_cpuid(void*) -> void;
	auto handle_execute_vmcall(void*) -> void;
	auto handle_execute_vmptrld(void*) -> void;
	auto handle_execute_vmptrst(void*) -> void;
	auto handle_execute_vmread(void*) -> void;
	auto handle_execute_vmwrite(void*) -> void;
	auto handle_execute_vmxon(void*) -> void;
	auto handle_mov_cr(void*) -> void;
	auto handle_mov_dr(void*) -> void;
	auto handle_execute_io_instruction(void*) -> void;
	auto handle_apic_access(void*) -> void;
	auto handle_gdtr_idtr_access(void*) -> void;
	auto handle_ldtr_tr_access(void*) -> void;
	auto handle_ept_violation(void*) -> void;
	auto handle_ept_misconfiguration(void*) -> void;
	auto handle_execute_invept(void*) -> void;
	auto handle_execute_invvpid(void*) -> void;
	auto handle_execute_rdrand(void*) -> void;
	auto handle_execute_invpcid(void*) -> void;
	auto handle_execute_xsaves(void*) -> void;
	auto handle_execute_xrstors(void*) -> void;
}

extern "C" {
	auto inline asm_inv_ept_global(unsigned __int64, struct _ept_error*) -> unsigned __int32;
	//inline EVmErrors AsmInveptContext();

	auto inline asm_vmx_vmcall(unsigned __int64, unsigned __int64, unsigned __int64, unsigned __int64) -> void;
}
