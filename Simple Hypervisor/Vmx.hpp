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

namespace vmx {
	auto vmx_is_vmx_available() -> bool;
	auto vmx_is_vmx_support() -> bool;
	auto vmx_check_bios_lock() -> bool;
	auto vmx_enable_cr4() -> void;

	auto create_vcpus() -> bool;

	auto vmx_allocate_vmxon_region(struct _vcpu_ctx*) -> bool;
	auto vmx_allocate_vmcs_region(struct _vcpu_ctx*) -> bool;
	auto vmx_allocate_vmexit_stack(uchar) -> bool;
	auto vmx_allocate_io_bitmap_stack(uchar) -> bool;
	auto vmx_allocate_msr_bitmap(uchar) -> bool;

	auto vmx_handle_vmcall(unsigned __int64, unsigned __int64, unsigned __int64, unsigned __int64) -> unsigned __int64;
}

namespace hv {
	auto virtualize_all_processors() -> bool;

	auto devirtualize_all_processors() -> void;

	auto launch_vm(ULONG_PTR) -> ULONG_PTR;
	auto terminate_vm(uchar) -> void;
	auto resume_vm() -> void;

	auto launch_all_vmms() -> void;

	auto launch_vm() -> void;

}

namespace vmexit {
	auto vmexit_handler(void* guest_regs) -> short;
}

extern "C" {
	auto inline asm_inv_ept_global(unsigned __int64, struct _ept_error*) -> unsigned __int32;
	//inline EVmErrors AsmInveptContext();

	auto inline asm_vmx_vmcall(unsigned __int64, unsigned __int64, unsigned __int64, unsigned __int64) -> void;
}
