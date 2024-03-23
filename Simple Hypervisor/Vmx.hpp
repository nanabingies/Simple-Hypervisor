#pragma once
#include <basetsd.h>
using uchar = unsigned char;

namespace vmx {
	auto vmx_is_vmx_available() -> bool;
	auto vmx_is_vmx_support() -> bool;
	auto vmx_check_bios_lock() -> bool;
	auto vmx_enable_cr4() -> void;

	auto vmx_allocate_vmxon_region(uchar) -> bool;
	auto vmx_allocate_vmcs_region(uchar) -> bool;
	auto vmx_allocate_vmexit_stack(uchar) -> bool;
	auto vmx_allocate_io_bitmap_stack(uchar) -> bool;
	auto vmx_allocate_msr_bitmap(uchar) -> bool;
}

namespace hv {
	auto virtualize_all_processors() -> bool;

	auto devirtualize_all_processors() -> void;

	auto launch_vm(ULONG_PTR) -> ULONG_PTR;
	auto terminate_vm(uchar) -> void;
	auto resume_vm(uchar) -> void;

	auto launch_all_vmms() -> void;

}

namespace vmexit {
	auto vmexit_handler(void* guest_regs) -> short;
}

extern "C" {
	inline EVmErrors asm_inv_ept_global(unsigned __int64, const ept_error*);
	inline EVmErrors AsmInveptContext();
}
