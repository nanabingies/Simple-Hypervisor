#include "stdafx.h"
#include "intrin.h"
#define VMCS_CTRL_TSC_OFFSET_HIGH	0x2011
#pragma warning(disable: 4244)

//
// https://github.com/ionescu007/SimpleVisor/blob/HEAD/shvutil.c#L25-L89 
// Thanks Alex Ionescu
//
VOID
ShvUtilConvertGdtEntry(
	_In_ VOID* GdtBase,
	_In_ UINT16 Selector,
	_Out_ pvmx_gdtentry64 VmxGdtEntry
)
{
	PKGDTENTRY64 gdtEntry;

	//
	// Reject LDT or NULL entries
	//
	if ((Selector == 0) ||
		(Selector & SELECTOR_TABLE_INDEX) != 0)
	{
		VmxGdtEntry->Limit = VmxGdtEntry->AccessRights = 0;
		VmxGdtEntry->Base = 0;
		VmxGdtEntry->Selector = 0;
		VmxGdtEntry->Bits.Unusable = TRUE;
		return;
	}

	//
	// Read the GDT entry at the given selector, masking out the RPL bits.
	//
	gdtEntry = (PKGDTENTRY64)((uintptr_t)GdtBase + (Selector & ~RPL_MASK)); // &0xF8

	//
	// Write the selector directly 
	//
	VmxGdtEntry->Selector = Selector;

	//
	// Use the LSL intrinsic to read the segment limit
	//
	VmxGdtEntry->Limit = __segmentlimit(Selector);

	//
	// Build the full 64-bit effective address, keeping in mind that only when
	// the System bit is unset, should this be done.
	//
	// NOTE: The Windows definition of KGDTENTRY64 is WRONG. The "System" field
	// is incorrectly defined at the position of where the AVL bit should be.
	// The actual location of the SYSTEM bit is encoded as the highest bit in
	// the "Type" field.
	//
	VmxGdtEntry->Base = ((gdtEntry->Bytes.BaseHigh << 24) |
		(gdtEntry->Bytes.BaseMiddle << 16) |
		(gdtEntry->BaseLow)) & 0xFFFFFFFF;
	VmxGdtEntry->Base |= ((gdtEntry->Bits.Type & 0x10) == 0) ?
		((uintptr_t)gdtEntry->BaseUpper << 32) : 0;

	//
	// Load the access rights
	//
	VmxGdtEntry->AccessRights = 0;
	VmxGdtEntry->Bytes.Flags1 = gdtEntry->Bytes.Flags1;
	VmxGdtEntry->Bytes.Flags2 = gdtEntry->Bytes.Flags2;

	//
	// Finally, handle the VMX-specific bits
	//
	VmxGdtEntry->Bits.Reserved = 0;
	VmxGdtEntry->Bits.Unusable = !gdtEntry->Bits.Present;
}

void fill_guest_selector_data(void* gdt_base, unsigned __int32 segment_register, unsigned __int16 selector) {
	__segment_access_rights segment_access_rights;
	__segment_descriptor* segment_descriptor;

	if (selector & 0x4)
		return;

	segment_descriptor = (__segment_descriptor*)((unsigned __int8*)gdt_base + (selector & ~0x7));

	unsigned __int64 segment_base = segment_descriptor->base_low | segment_descriptor->base_middle << 16 | segment_descriptor->base_high << 24;

	unsigned __int32 segment_limit = segment_descriptor->limit_low | (segment_descriptor->segment_limit_high << 16);

	//
	// Load ar get access rights of descriptor specified by selector
	// Lower 8 bits are zeroed so we have to bit shift it to right by 8
	//
	segment_access_rights.all = __load_ar(selector) >> 8;
	segment_access_rights.unusable = 0;
	segment_access_rights.reserved0 = 0;
	segment_access_rights.reserved1 = 0;

	// This is a TSS or callgate etc, save the base high part
	if (segment_descriptor->descriptor_type == false)
		segment_base = (segment_base & MASK_32BITS) | (unsigned __int64)segment_descriptor->base_upper << 32;

	if (segment_descriptor->granularity == true)
		segment_limit = (segment_limit << 12) + 0xfff;

	if (selector == 0)
		segment_access_rights.all |= 0x10000;

	__vmx_vmwrite(VMCS_GUEST_ES_SELECTOR + segment_register * 2, selector);
	__vmx_vmwrite(VMCS_GUEST_ES_LIMIT + segment_register * 2, segment_limit);
	__vmx_vmwrite(VMCS_GUEST_ES_BASE + segment_register * 2, segment_base);
	__vmx_vmwrite(VMCS_GUEST_ES_ACCESS_RIGHTS + segment_register * 2, segment_access_rights.all);
}

auto get_segment_base(unsigned __int16 selector, unsigned __int8* gdt_base) -> unsigned __int64 {
	__segment_descriptor* segment_descriptor;

	segment_descriptor = (__segment_descriptor*)(gdt_base + (selector & ~0x7));

	unsigned __int64 segment_base = segment_descriptor->base_low | segment_descriptor->base_middle << 16 | segment_descriptor->base_high << 24;

	if (segment_descriptor->descriptor_type == false)
		segment_base = (segment_base & MASK_32BITS) | (unsigned __int64)segment_descriptor->base_upper << 32;

	return segment_base;
}

auto adjust_controls(ulong Ctl, ulong Msr) -> unsigned __int64 {
	LARGE_INTEGER MsrValue = { 0 };

	MsrValue.QuadPart = __readmsr(Msr);
	Ctl &= MsrValue.HighPart;	/* bit == 0 in high word ==> must be zero */
	Ctl |= MsrValue.LowPart;	/* bit == 1 in low word  ==> must be one  */

	return Ctl;
}

void set_io_bitmap(unsigned __int16 io_port, __vcpu* vcpu, bool value) {
	unsigned __int16 bitmap_position;
	unsigned __int8 bitmap_bit;

	//
	// IO ports from 0x8000 to 0xFFFF are encoded in io bitmap b
	if (io_port >= 0x8000) {
		io_port -= 0x8000;
		bitmap_position = io_port / 8;
		bitmap_bit = io_port % 8;

		if (value == true)
			*(vcpu->vcpu_bitmaps.io_bitmap_b + bitmap_position) |= (1 << bitmap_bit);
		else
			*(vcpu->vcpu_bitmaps.io_bitmap_b + bitmap_position) &= ~(1 << bitmap_bit);
	}

	//
	// IO ports from 0 to 0x7fff are encoded in io bitmap b
	else {
		bitmap_position = io_port / 8;
		bitmap_bit = io_port % 8;

		if (value == true)
			*(vcpu->vcpu_bitmaps.io_bitmap_a + bitmap_position) |= (1 << bitmap_bit);
		else
			*(vcpu->vcpu_bitmaps.io_bitmap_a + bitmap_position) &= ~(1 << bitmap_bit);
	}
}

auto set_exception_bitmap(__exception_bitmap& exception_bitmap) -> void {
	exception_bitmap.divide_error = false;

	exception_bitmap.debug = true;

	exception_bitmap.nmi_interrupt = false;

	exception_bitmap.breakpoint = false;

	exception_bitmap.overflow = false;

	exception_bitmap.bound = false;

	exception_bitmap.invalid_opcode = false;

	exception_bitmap.coprocessor_segment_overrun = false;

	exception_bitmap.invalid_tss = false;

	exception_bitmap.segment_not_present = false;

	exception_bitmap.stack_segment_fault = false;

	exception_bitmap.general_protection = false;

	exception_bitmap.page_fault = false;

	exception_bitmap.x87_floating_point_error = false;

	exception_bitmap.alignment_check = false;

	exception_bitmap.machine_check = false;

	exception_bitmap.simd_floating_point_error = false;

	exception_bitmap.virtualization_exception = false;
}

auto save_vmentry_fields(ia32_vmx_entry_ctls_register& entry_ctls) -> void {
	entry_ctls.load_debug_controls = true;
	entry_ctls.ia32e_mode_guest = true;
	entry_ctls.entry_to_smm = false;
	entry_ctls.deactivate_dual_monitor_treatment = false;
	entry_ctls.load_ia32_perf_global_ctrl = false;
	entry_ctls.load_ia32_pat = false;
	entry_ctls.load_ia32_efer = false;
	entry_ctls.load_ia32_bndcfgs = false;
	entry_ctls.conceal_vmx_from_pt = true;
	entry_ctls.load_cet_state = false;
	entry_ctls.load_ia32_lbr_ctl = false;
	entry_ctls.load_ia32_pkrs = false;
}

auto save_vmexit_fields(ia32_vmx_exit_ctls_register& exit_ctls) -> void {
	exit_ctls.save_debug_controls = true;
	exit_ctls.host_address_space_size = true;
	exit_ctls.load_ia32_perf_global_ctrl = false;
	exit_ctls.acknowledge_interrupt_on_exit = true;
	exit_ctls.load_ia32_pat = false;
	exit_ctls.save_ia32_pat = false;
	exit_ctls.save_ia32_efer = false;
	exit_ctls.load_ia32_efer = false;
	exit_ctls.save_vmx_preemption_timer_value = false;
	exit_ctls.clear_ia32_bndcfgs = false;
	exit_ctls.conceal_vmx_from_pt = true;
	exit_ctls.clear_ia32_rtit_ctl = false;
	exit_ctls.clear_ia32_lbr_ctl = false;
	exit_ctls.load_ia32_cet_state = false;
	exit_ctls.load_ia32_pkrs = false;
}

auto save_proc_based_fields(ia32_vmx_procbased_ctls_register& procbased_ctls) -> void {
	procbased_ctls.interrupt_window_exiting = false;
	procbased_ctls.use_tsc_offsetting = true;
	procbased_ctls.hlt_exiting = true;
	procbased_ctls.invlpg_exiting = true;
	procbased_ctls.mwait_exiting = false;
	procbased_ctls.rdpmc_exiting = false;
	procbased_ctls.rdtsc_exiting = true;
	procbased_ctls.cr3_load_exiting = true;
	procbased_ctls.cr3_store_exiting = true;
	procbased_ctls.activate_tertiary_controls = false;
	procbased_ctls.cr8_load_exiting = false;
	procbased_ctls.cr8_store_exiting = false;
	procbased_ctls.use_tpr_shadow = false;
	procbased_ctls.nmi_window_exiting = false;
	procbased_ctls.mov_dr_exiting = true;
	procbased_ctls.unconditional_io_exiting = false;
	procbased_ctls.use_io_bitmaps = true;
	procbased_ctls.monitor_trap_flag = false;
	procbased_ctls.use_msr_bitmaps = true;
	procbased_ctls.monitor_exiting = false;
	procbased_ctls.pause_exiting = false;
	procbased_ctls.activate_secondary_controls = true;
}

auto save_proc_secondary_fields(ia32_vmx_procbased_ctls2_register& procbased_ctls2) -> void {
	procbased_ctls2.virtualize_apic_accesses = false;
	procbased_ctls2.enable_ept = false;
	procbased_ctls2.descriptor_table_exiting = true;
	procbased_ctls2.enable_rdtscp = true;
	procbased_ctls2.virtualize_x2apic_mode = false;
	procbased_ctls2.enable_vpid = true;
	procbased_ctls2.wbinvd_exiting = true;
	procbased_ctls2.unrestricted_guest = false;
	procbased_ctls2.apic_register_virtualization = false;
	procbased_ctls2.virtual_interrupt_delivery = false;
	procbased_ctls2.pause_loop_exiting = false;
	procbased_ctls2.rdrand_exiting = true;
	procbased_ctls2.enable_invpcid = true;
	procbased_ctls2.enable_vm_functions = false;
	procbased_ctls2.vmcs_shadowing = false;
	procbased_ctls2.enable_encls_exiting = false;
	procbased_ctls2.rdseed_exiting = true;
	procbased_ctls2.enable_pml = false;
	procbased_ctls2.ept_violation = true;
	procbased_ctls2.conceal_vmx_from_pt = true;
	procbased_ctls2.enable_xsaves = true;
	procbased_ctls2.mode_based_execute_control_for_ept = true;
	procbased_ctls2.sub_page_write_permissions_for_ept = false;
	procbased_ctls2.pt_uses_guest_physical_addresses = false;
	procbased_ctls2.use_tsc_scaling = false;
	procbased_ctls2.enable_user_wait_pause = false;
	procbased_ctls2.enable_enclv_exiting = false;
}

auto save_pin_fields(ia32_vmx_pinbased_ctls_register& pinbased_ctls) -> void {
	pinbased_ctls.external_interrupt_exiting = false;
	pinbased_ctls.nmi_exiting = false;
	pinbased_ctls.virtual_nmi = false;
	pinbased_ctls.activate_vmx_preemption_timer = false;
	pinbased_ctls.process_posted_interrupts = false;
}

void dump_vmcs() {
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "-----------------------------------VMCS CORE %u DUMP-----------------------------------\r\n", KeGetCurrentProcessorIndex());

	size_t parm = 0;
	// Natural Guest Register State Fields
	__vmx_vmread(VMCS_GUEST_CR0, &parm);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_CR0: 0x%llX", parm);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_CR3: 0x%llX", vmread(GUEST_CR3));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_CR4: 0x%llX", vmread(GUEST_CR4));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_ES_BASE: 0x%llX", vmread(GUEST_ES_BASE));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_CS_BASE: 0x%llX", vmread(GUEST_CS_BASE));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_SS_BASE: 0x%llX", vmread(GUEST_SS_BASE));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_DS_BASE: 0x%llX", vmread(GUEST_DS_BASE));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_FS_BASE: 0x%llX", vmread(GUEST_FS_BASE));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_GS_BASE: 0x%llX", vmread(GUEST_GS_BASE));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_LDTR_BASE: 0x%llX", vmread(GUEST_LDTR_BASE));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_TR_BASE: 0x%llX", vmread(GUEST_TR_BASE));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_GDTR_BASE: 0x%llX", vmread(GUEST_GDTR_BASE));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_IDTR_BASE: 0x%llX", vmread(GUEST_IDTR_BASE));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_DR7: 0x%llX", vmread(GUEST_DR7));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_RSP: 0x%llX", vmread(GUEST_RSP));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_RIP: 0x%llX", vmread(GUEST_RIP));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_RFLAGS: 0x%llX", vmread(GUEST_RFLAGS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_SYSENTER_ESP: 0x%llX", vmread(GUEST_SYSENTER_ESP));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_SYSENTER_EIP: 0x%llX", vmread(GUEST_SYSENTER_EIP));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_S_CET: 0x%llX", vmread(GUEST_S_CET));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_SSP: 0x%llX", vmread(GUEST_SSP));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_INTERRUPT_SSP_TABLE_ADDR: 0x%llX", vmread(GUEST_INTERRUPT_SSP_TABLE_ADDR));

	// 64-bit Guest Register State Fields
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_VMCS_LINK_POINTER: 0x%llX", vmread(GUEST_VMCS_LINK_POINTER));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_DEBUG_CONTROL: 0x%llX", vmread(GUEST_DEBUG_CONTROL));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_PAT: 0x%llX", vmread(GUEST_PAT));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_EFER: 0x%llX", vmread(GUEST_EFER));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_PERF_GLOBAL_CONTROL: 0x%llX", vmread(GUEST_PERF_GLOBAL_CONTROL));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_PDPTE0: 0x%llX", vmread(GUEST_PDPTE0));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_PDPTE1: 0x%llX", vmread(GUEST_PDPTE1));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_PDPTE2: 0x%llX", vmread(GUEST_PDPTE2));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_PDPTE3: 0x%llX", vmread(GUEST_PDPTE3));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_BNDCFGS: 0x%llX", vmread(GUEST_BNDCFGS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_RTIT_CTL: 0x%llX", vmread(GUEST_RTIT_CTL));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_PKRS: 0x%llX", vmread(GUEST_PKRS));

	// 32-Bit Guest Register State Fields
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_ES_LIMIT: 0x%llX", vmread(GUEST_ES_LIMIT));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_CS_LIMIT: 0x%llX", vmread(GUEST_CS_LIMIT));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_SS_LIMIT: 0x%llX", vmread(GUEST_SS_LIMIT));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_DS_LIMIT: 0x%llX", vmread(GUEST_DS_LIMIT));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_FS_LIMIT: 0x%llX", vmread(GUEST_FS_LIMIT));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_GS_LIMIT: 0x%llX", vmread(GUEST_GS_LIMIT));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_LDTR_LIMIT: 0x%llX", vmread(GUEST_LDTR_LIMIT));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_TR_LIMIT: 0x%llX", vmread(GUEST_TR_LIMIT));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_GDTR_LIMIT: 0x%llX", vmread(GUEST_GDTR_LIMIT));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_IDTR_LIMIT: 0x%llX", vmread(GUEST_IDTR_LIMIT));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_ES_ACCESS_RIGHTS: 0x%llX", vmread(GUEST_ES_ACCESS_RIGHTS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_CS_ACCESS_RIGHTS: 0x%llX", vmread(GUEST_CS_ACCESS_RIGHTS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_SS_ACCESS_RIGHTS: 0x%llX", vmread(GUEST_SS_ACCESS_RIGHTS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_DS_ACCESS_RIGHTS: 0x%llX", vmread(GUEST_DS_ACCESS_RIGHTS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_FS_ACCESS_RIGHTS: 0x%llX", vmread(GUEST_FS_ACCESS_RIGHTS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_GS_ACCESS_RIGHTS: 0x%llX", vmread(GUEST_GS_ACCESS_RIGHTS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_LDTR_ACCESS_RIGHTS: 0x%llX", vmread(GUEST_LDTR_ACCESS_RIGHTS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_TR_ACCESS_RIGHTS: 0x%llX", vmread(GUEST_TR_ACCESS_RIGHTS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_INTERRUPTIBILITY_STATE: 0x%llX", vmread(GUEST_INTERRUPTIBILITY_STATE));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_ACTIVITY_STATE: 0x%llX", vmread(GUEST_ACTIVITY_STATE));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_SMBASE: 0x%llX", vmread(GUEST_SMBASE));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_SYSENTER_CS: 0x%llX", vmread(GUEST_SYSENTER_CS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_VMX_PREEMPTION_TIMER_VALUE: 0x%llX", vmread(GUEST_VMX_PREEMPTION_TIMER_VALUE));

	// 16-Bit Guest Register State Fields
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_ES_SELECTOR: 0x%llX", vmread(GUEST_ES_SELECTOR));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_CS_SELECTOR: 0x%llX", vmread(GUEST_CS_SELECTOR));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_SS_SELECTOR: 0x%llX", vmread(GUEST_SS_SELECTOR));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_DS_SELECTOR: 0x%llX", vmread(GUEST_DS_SELECTOR));
	LogDump("GUEST_FS_SELECTOR: 0x%llX", vmread(GUEST_FS_SELECTOR));
	LogDump("GUEST_GS_SELECTOR: 0x%llX", vmread(GUEST_GS_SELECTOR));
	LogDump("GUEST_LDTR_SELECTOR: 0x%llX", vmread(GUEST_LDTR_SELECTOR));
	LogDump("GUEST_TR_SELECTOR: 0x%llX", vmread(GUEST_TR_SELECTOR));
	LogDump("GUEST_GUEST_INTERRUPT_STATUS: 0x%llX", vmread(GUEST_GUEST_INTERRUPT_STATUS));
	LogDump("GUEST_PML_INDEX: 0x%llX", vmread(GUEST_PML_INDEX));

	// Natural Host Register State Fields
	LogDump("HOST_CR0: 0x%llX", vmread(HOST_CR0));
	LogDump("HOST_CR3: 0x%llX", vmread(HOST_CR3));
	LogDump("HOST_CR4: 0x%llX", vmread(HOST_CR4));
	LogDump("HOST_FS_BASE: 0x%llX", vmread(HOST_FS_BASE));
	LogDump("HOST_GS_BASE: 0x%llX", vmread(HOST_GS_BASE));
	LogDump("HOST_TR_BASE: 0x%llX", vmread(HOST_TR_BASE));
	LogDump("HOST_GDTR_BASE: 0x%llX", vmread(HOST_GDTR_BASE));
	LogDump("HOST_IDTR_BASE: 0x%llX", vmread(HOST_IDTR_BASE));
	LogDump("HOST_SYSENTER_ESP: 0x%llX", vmread(HOST_SYSENTER_ESP));
	LogDump("HOST_SYSENTER_EIP: 0x%llX", vmread(HOST_SYSENTER_EIP));
	LogDump("HOST_RSP: 0x%llX", vmread(HOST_RSP));
	LogDump("HOST_RIP: 0x%llX", vmread(HOST_RIP));
	LogDump("HOST_S_CET: 0x%llX", vmread(HOST_S_CET));
	LogDump("HOST_SSP: 0x%llX", vmread(HOST_SSP));
	LogDump("HOST_INTERRUPT_SSP_TABLE_ADDR: 0x%llX", vmread(HOST_INTERRUPT_SSP_TABLE_ADDR));

	// 64-bit Host Register State Fields
	LogDump("HOST_PAT: 0x%llX", vmread(HOST_PAT));
	LogDump("HOST_EFER: 0x%llX", vmread(HOST_EFER));
	LogDump("HOST_PERF_GLOBAL_CTRL: 0x%llX", vmread(HOST_PERF_GLOBAL_CTRL));
	LogDump("HOST_PKRS: 0x%llX", vmread(HOST_PKRS));

	// 32-bit Host Register State Fields
	LogDump("HOST_SYSENTER_CS: 0x%llX", vmread(HOST_SYSENTER_CS));

	// 16-bit Host Register State Fields
	LogDump("HOST_ES_SELECTOR: 0x%llX", vmread(HOST_ES_SELECTOR));
	LogDump("HOST_CS_SELECTOR: 0x%llX", vmread(HOST_CS_SELECTOR));
	LogDump("HOST_SS_SELECTOR: 0x%llX", vmread(HOST_SS_SELECTOR));
	LogDump("HOST_DS_SELECTOR: 0x%llX", vmread(HOST_DS_SELECTOR));
	LogDump("HOST_FS_SELECTOR: 0x%llX", vmread(HOST_FS_SELECTOR));
	LogDump("HOST_GS_SELECTOR: 0x%llX", vmread(HOST_GS_SELECTOR));
	LogDump("HOST_TR_SELECTOR: 0x%llX", vmread(HOST_TR_SELECTOR));

	// Natural Control Register State Fields
	LogDump("CONTROL_CR0_GUEST_HOST_MASK: 0x%llX", vmread(CONTROL_CR0_GUEST_HOST_MASK));
	LogDump("CONTROL_CR4_GUEST_HOST_MASK: 0x%llX", vmread(CONTROL_CR4_GUEST_HOST_MASK));
	LogDump("CONTROL_CR0_READ_SHADOW: 0x%llX", vmread(CONTROL_CR0_READ_SHADOW));
	LogDump("CONTROL_CR4_READ_SHADOW: 0x%llX", vmread(CONTROL_CR4_READ_SHADOW));
	LogDump("CONTROL_CR3_TARGET_VALUE_0: 0x%llX", vmread(CONTROL_CR3_TARGET_VALUE_0));
	LogDump("CONTROL_CR3_TARGET_VALUE_1: 0x%llX", vmread(CONTROL_CR3_TARGET_VALUE_1));
	LogDump("CONTROL_CR3_TARGET_VALUE_2: 0x%llX", vmread(CONTROL_CR3_TARGET_VALUE_2));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_CR3_TARGET_VALUE_3: 0x%llX", vmread(CONTROL_CR3_TARGET_VALUE_3));

	// 64-bit Control Register State Fields
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_BITMAP_IO_A_ADDRESS: 0x%llX", vmread(CONTROL_BITMAP_IO_A_ADDRESS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_BITMAP_IO_B_ADDRESS: 0x%llX", vmread(CONTROL_BITMAP_IO_B_ADDRESS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_MSR_BITMAPS_ADDRESS: 0x%llX", vmread(CONTROL_MSR_BITMAPS_ADDRESS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_VMEXIT_MSR_STORE_ADDRESS: 0x%llX", vmread(CONTROL_VMEXIT_MSR_STORE_ADDRESS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_VMEXIT_MSR_LOAD_ADDRESS: 0x%llX", vmread(CONTROL_VMEXIT_MSR_LOAD_ADDRESS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_VMENTER_MSR_LOAD_ADDRESS: 0x%llX", vmread(CONTROL_VMENTER_MSR_LOAD_ADDRESS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_VMCS_EXECUTIVE_POINTER: 0x%llX", vmread(CONTROL_VMCS_EXECUTIVE_POINTER));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_PML_ADDRESS: 0x%llX", vmread(CONTROL_PML_ADDRESS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_TSC_OFFSET: 0x%llX", vmread(CONTROL_TSC_OFFSET));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_VIRTUAL_APIC_ADDRESS: 0x%llX", vmread(CONTROL_VIRTUAL_APIC_ADDRESS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_APIC_ACCESS_ADDRESS: 0x%llX", vmread(CONTROL_APIC_ACCESS_ADDRESS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_POSTED_INTERRUPT_DESCRIPTOR_ADDRESS: 0x%llX", vmread(CONTROL_POSTED_INTERRUPT_DESCRIPTOR_ADDRESS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_VM_FUNCTION_CONTROLS: 0x%llX", vmread(CONTROL_VM_FUNCTION_CONTROLS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_EPT_POINTER: 0x%llX", vmread(CONTROL_EPT_POINTER));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_EOI_EXIT_BITMAP_0: 0x%llX", vmread(CONTROL_EOI_EXIT_BITMAP_0));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_EOI_EXIT_BITMAP_1: 0x%llX", vmread(CONTROL_EOI_EXIT_BITMAP_1));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_EOI_EXIT_BITMAP_2: 0x%llX", vmread(CONTROL_EOI_EXIT_BITMAP_2));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_EOI_EXIT_BITMAP_3: 0x%llX", vmread(CONTROL_EOI_EXIT_BITMAP_3));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_EPTP_LIST_ADDRESS: 0x%llX", vmread(CONTROL_EPTP_LIST_ADDRESS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_VMREAD_BITMAP_ADDRESS: 0x%llX", vmread(CONTROL_VMREAD_BITMAP_ADDRESS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_VMWRITE_BITMAP_ADDRESS: 0x%llX", vmread(CONTROL_VMWRITE_BITMAP_ADDRESS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_VIRTUALIZATION_EXCEPTION_INFORMATION_ADDRESS: 0x%llX", vmread(CONTROL_VIRTUALIZATION_EXCEPTION_INFORMATION_ADDRESS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_XSS_EXITING_BITMAP: 0x%llX", vmread(CONTROL_XSS_EXITING_BITMAP));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_ENCLS_EXITING_BITMAP: 0x%llX", vmread(CONTROL_ENCLS_EXITING_BITMAP));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_SUB_PAGE_PERMISSION_TABLE_POINTER: 0x%llX", vmread(CONTROL_SUB_PAGE_PERMISSION_TABLE_POINTER));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_TSC_MULTIPLIER: 0x%llX", vmread(CONTROL_TSC_MULTIPLIER));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_ENCLV_EXITING_BITMAP: 0x%llX", vmread(CONTROL_ENCLV_EXITING_BITMAP));

	// 32-bit Control Register State Fields
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_PIN_BASED_VM_EXECUTION_CONTROLS: 0x%llX", vmread(CONTROL_PIN_BASED_VM_EXECUTION_CONTROLS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_PRIMARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS: 0x%llX", vmread(CONTROL_PRIMARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_EXCEPTION_BITMAP: 0x%llX", vmread(CONTROL_EXCEPTION_BITMAP));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_PAGE_FAULT_ERROR_CODE_MASK: 0x%llX", vmread(CONTROL_PAGE_FAULT_ERROR_CODE_MASK));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_PAGE_FAULT_ERROR_CODE_MATCH: 0x%llX", vmread(CONTROL_PAGE_FAULT_ERROR_CODE_MATCH));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_CR3_TARGET_COUNT: 0x%llX", vmread(CONTROL_CR3_TARGET_COUNT));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_VM_EXIT_CONTROLS: 0x%llX", vmread(CONTROL_VM_EXIT_CONTROLS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_VM_EXIT_MSR_STORE_COUNT: 0x%llX", vmread(CONTROL_VM_EXIT_MSR_STORE_COUNT));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_VM_EXIT_MSR_LOAD_COUNT: 0x%llX", vmread(CONTROL_VM_EXIT_MSR_LOAD_COUNT));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_VM_ENTRY_CONTROLS: 0x%llX", vmread(CONTROL_VM_ENTRY_CONTROLS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_VM_ENTRY_MSR_LOAD_COUNT: 0x%llX", vmread(CONTROL_VM_ENTRY_MSR_LOAD_COUNT));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_VM_ENTRY_INTERRUPTION_INFORMATION_FIELD: 0x%llX", vmread(CONTROL_VM_ENTRY_INTERRUPTION_INFORMATION_FIELD));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_VM_ENTRY_EXCEPTION_ERROR_CODE: 0x%llX", vmread(CONTROL_VM_ENTRY_EXCEPTION_ERROR_CODE));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_VM_ENTRY_INSTRUCTION_LENGTH: 0x%llX", vmread(CONTROL_VM_ENTRY_INSTRUCTION_LENGTH));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_TPR_THRESHOLD: 0x%llX", vmread(CONTROL_TPR_THRESHOLD));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS: 0x%llX", vmread(CONTROL_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_PLE_GAP: 0x%llX", vmread(CONTROL_PLE_GAP));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_PLE_WINDOW: 0x%llX", vmread(CONTROL_PLE_WINDOW));

	// 16-bit Control Register State Fields
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_VIRTUAL_PROCESSOR_IDENTIFIER: 0x%llX", vmread(CONTROL_VIRTUAL_PROCESSOR_IDENTIFIER));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_POSTED_INTERRUPT_NOTIFICATION_VECTOR: 0x%llX", vmread(CONTROL_POSTED_INTERRUPT_NOTIFICATION_VECTOR));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_EPTP_INDEX: 0x%llX", vmread(CONTROL_EPTP_INDEX));

	// Natural Read only Register State Fields
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "EXIT_QUALIFICATION: 0x%llX", vmread(EXIT_QUALIFICATION));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IO_RCX: 0x%llX", vmread(IO_RCX));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IO_RSI: 0x%llX", vmread(IO_RSI));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IO_RDI: 0x%llX", vmread(IO_RDI));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IO_RIP: 0x%llX", vmread(IO_RIP));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_LINEAR_ADDRESS: 0x%llX", vmread(GUEST_LINEAR_ADDRESS));

	// 64-bit Read only Register State Fields
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_PHYSICAL_ADDRESS: 0x%llX", vmread(GUEST_PHYSICAL_ADDRESS));

	// 32-bit Read only Register State Fields
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "VM_INSTRUCTION_ERROR: 0x%llX", vmread(VM_INSTRUCTION_ERROR));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "EXIT_REASON: 0x%llX", vmread(EXIT_REASON));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "VM_EXIT_INTERRUPTION_INFORMATION: 0x%llX", vmread(VM_EXIT_INTERRUPTION_INFORMATION));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "VM_EXIT_INTERRUPTION_ERROR_CODE: 0x%llX", vmread(VM_EXIT_INTERRUPTION_ERROR_CODE));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IDT_VECTORING_INFORMATION_FIELD: 0x%llX", vmread(IDT_VECTORING_INFORMATION_FIELD));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IDT_VECTORING_ERROR_CODE: 0x%llX", vmread(IDT_VECTORING_ERROR_CODE));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "VM_EXIT_INSTRUCTION_LENGTH: 0x%llX", vmread(VM_EXIT_INSTRUCTION_LENGTH));
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "VM_EXIT_INSTRUCTION_INFORMATION: 0x%llX", vmread(VM_EXIT_INSTRUCTION_INFORMATION));

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "-----------------------------------VMCS CORE %u DUMP-----------------------------------\r\n", KeGetCurrentProcessorIndex());
}

auto hv_setup_vmcs(struct __vcpu* vcpu, void* guest_rsp) -> void {
	ia32_vmx_basic_register vmx_basic{};
	vmx_basic.flags = __readmsr(IA32_VMX_BASIC);

	/// VM-ENTRY CONTROL FIELDS
	ia32_vmx_entry_ctls_register entry_ctls{};
	save_vmentry_fields(entry_ctls);
	
	/// VM-EXIT INFORMATION FIELDS
	ia32_vmx_exit_ctls_register exit_ctls{};
	save_vmexit_fields(exit_ctls);
	
	/// Primary Processor-Based VM-Execution Controls.
	ia32_vmx_procbased_ctls_register procbased_ctls{};
	save_proc_based_fields(procbased_ctls);

	/// Secondary Processor-Based VM-Execution Controls
	ia32_vmx_procbased_ctls2_register procbased_ctls2{};
	save_proc_secondary_fields(procbased_ctls2);

	/// Pin-Based VM-Execution Controls
	ia32_vmx_pinbased_ctls_register pinbased_ctls{};
	save_pin_fields(pinbased_ctls);

	memset(vcpu->vcpu_bitmaps.io_bitmap_a, 0xff, PAGE_SIZE);
	memset(vcpu->vcpu_bitmaps.io_bitmap_b, 0xff, PAGE_SIZE);
	memset(vcpu->vcpu_bitmaps.msr_bitmap, 0xff, PAGE_SIZE);

	/// Set VMCS state to inactive
	__vmx_vmclear(static_cast<unsigned long long*>(&vcpu->vmcs_physical));

	/// Make VMCS the current and active on that processor
	__vmx_vmptrld(static_cast<unsigned long long*>(&vcpu->vmcs_physical));

	/// Control Registers - Guest & Host
	if (__vmx_vmwrite(VMCS_GUEST_CR0, __readcr0()))	return;
	if (__vmx_vmwrite(VMCS_GUEST_CR3, __readcr3()))	return;
	if (__vmx_vmwrite(VMCS_GUEST_CR4, __readcr4()))	return;

	if (__vmx_vmwrite(VMCS_HOST_CR0, __readcr0()))	return;
	if (__vmx_vmwrite(VMCS_HOST_CR3, __readcr3()))	return;		// Host cr3		// hv::get_system_dirbase()
	if (__vmx_vmwrite(VMCS_HOST_CR4, __readcr4()))	return;

	if (__vmx_vmwrite(VMCS_CTRL_CR0_READ_SHADOW, __readcr0()))	return;
	if (__vmx_vmwrite(VMCS_CTRL_CR4_READ_SHADOW, __readcr4()))	return;

	if (__vmx_vmwrite(VMCS_CTRL_CR3_TARGET_COUNT, __readcr3()))	return;
	if (__vmx_vmwrite(VMCS_CTRL_CR4_READ_SHADOW, __readcr4()))	return;
	if (__vmx_vmwrite(VMCS_CTRL_CR4_GUEST_HOST_MASK, 0x2000))	return;

	/// Debug Register (DR7)
	if (__vmx_vmwrite(VMCS_GUEST_DR7, __readdr(7)))	return;

	/// RSP, RIP, RFLAGS - Guest & Host
	if (__vmx_vmwrite(VMCS_GUEST_RSP, reinterpret_cast<size_t>(guest_rsp)))	return;
	if (__vmx_vmwrite(VMCS_GUEST_RIP, reinterpret_cast<size_t>(asm_restore_vmm_state)))	return;
	if (__vmx_vmwrite(VMCS_GUEST_RFLAGS, __readeflags())) return;

	if (__vmx_vmwrite(VMCS_HOST_RSP, reinterpret_cast<size_t>(vcpu->vmm_stack) + HOST_STACK_SIZE))	return;
	// Address host should point to, to kick things off when vmexit occurs
	if (__vmx_vmwrite(VMCS_HOST_RIP, reinterpret_cast<size_t>(asm_host_continue_execution)))	return;


	/// CS, SS, DS, ES, FS, GS, LDTR, and TR -- Guest & Host
	CONTEXT ctx;
	RtlCaptureContext(&ctx);

	uint64_t gdtBase = asm_get_gdt_base();
	vmx_gdtentry64 vmxGdtEntry;

	ShvUtilConvertGdtEntry(reinterpret_cast<void*>(gdtBase), ctx.SegCs, &vmxGdtEntry);

	__vmx_vmwrite(VMCS_GUEST_CS_SELECTOR, vmxGdtEntry.Selector);
	__vmx_vmwrite(VMCS_GUEST_CS_BASE, vmxGdtEntry.Base);
	__vmx_vmwrite(VMCS_GUEST_CS_LIMIT, vmxGdtEntry.Limit);
	__vmx_vmwrite(VMCS_GUEST_CS_ACCESS_RIGHTS, vmxGdtEntry.AccessRights);

	__vmx_vmwrite(VMCS_HOST_CS_SELECTOR, (ctx.SegCs & 0xF8));

	ShvUtilConvertGdtEntry(reinterpret_cast<void*>(gdtBase), ctx.SegSs, &vmxGdtEntry);

	__vmx_vmwrite(VMCS_GUEST_SS_SELECTOR, vmxGdtEntry.Selector);
	__vmx_vmwrite(VMCS_GUEST_SS_BASE, vmxGdtEntry.Base);
	__vmx_vmwrite(VMCS_GUEST_SS_LIMIT, vmxGdtEntry.Limit);
	__vmx_vmwrite(VMCS_GUEST_SS_ACCESS_RIGHTS, vmxGdtEntry.AccessRights);

	__vmx_vmwrite(VMCS_HOST_SS_SELECTOR, (ctx.SegSs & 0xF8));

	ShvUtilConvertGdtEntry(reinterpret_cast<void*>(gdtBase), ctx.SegDs, &vmxGdtEntry);

	__vmx_vmwrite(VMCS_GUEST_DS_SELECTOR, vmxGdtEntry.Selector);
	__vmx_vmwrite(VMCS_GUEST_DS_BASE, vmxGdtEntry.Base);
	__vmx_vmwrite(VMCS_GUEST_DS_LIMIT, vmxGdtEntry.Limit);
	__vmx_vmwrite(VMCS_GUEST_DS_ACCESS_RIGHTS, vmxGdtEntry.AccessRights);

	__vmx_vmwrite(VMCS_HOST_DS_SELECTOR, (ctx.SegDs & 0xF8));

	ShvUtilConvertGdtEntry(reinterpret_cast<void*>(gdtBase), ctx.SegEs, &vmxGdtEntry);

	__vmx_vmwrite(VMCS_GUEST_ES_SELECTOR, vmxGdtEntry.Selector);	// GETES() & 0xF8
	__vmx_vmwrite(VMCS_GUEST_ES_BASE, vmxGdtEntry.Base);
	__vmx_vmwrite(VMCS_GUEST_ES_LIMIT, vmxGdtEntry.Limit);
	__vmx_vmwrite(VMCS_GUEST_ES_ACCESS_RIGHTS, vmxGdtEntry.AccessRights);

	__vmx_vmwrite(VMCS_HOST_ES_SELECTOR, (ctx.SegEs & 0xF8));

	ShvUtilConvertGdtEntry(reinterpret_cast<void*>(gdtBase), ctx.SegFs, &vmxGdtEntry);

	__vmx_vmwrite(VMCS_GUEST_FS_SELECTOR, vmxGdtEntry.Selector);
	__vmx_vmwrite(VMCS_GUEST_FS_BASE, __readmsr(IA32_FS_BASE));
	__vmx_vmwrite(VMCS_GUEST_FS_LIMIT, vmxGdtEntry.Limit);
	__vmx_vmwrite(VMCS_GUEST_FS_ACCESS_RIGHTS, vmxGdtEntry.AccessRights);

	__vmx_vmwrite(VMCS_HOST_FS_SELECTOR, (ctx.SegFs & 0xF8));
	__vmx_vmwrite(VMCS_HOST_FS_BASE, __readmsr(IA32_FS_BASE));
	__vmx_vmwrite(VMCS_GUEST_FS_BASE, __readmsr(IA32_FS_BASE));

	ShvUtilConvertGdtEntry(reinterpret_cast<void*>(gdtBase), ctx.SegGs, &vmxGdtEntry);

	__vmx_vmwrite(VMCS_GUEST_GS_SELECTOR, vmxGdtEntry.Selector);
	__vmx_vmwrite(VMCS_GUEST_GS_BASE, __readmsr(IA32_GS_BASE));
	__vmx_vmwrite(VMCS_GUEST_GS_LIMIT, vmxGdtEntry.Limit);
	__vmx_vmwrite(VMCS_GUEST_GS_ACCESS_RIGHTS, vmxGdtEntry.AccessRights);

	__vmx_vmwrite(VMCS_HOST_GS_SELECTOR, (ctx.SegGs & 0xF8));
	__vmx_vmwrite(VMCS_HOST_GS_BASE, __readmsr(IA32_GS_BASE));
	__vmx_vmwrite(VMCS_GUEST_GS_BASE, __readmsr(IA32_GS_BASE));


	ShvUtilConvertGdtEntry(reinterpret_cast<void*>(gdtBase), asm_get_ldtr(), &vmxGdtEntry);

	__vmx_vmwrite(VMCS_GUEST_LDTR_SELECTOR, vmxGdtEntry.Selector);
	__vmx_vmwrite(VMCS_GUEST_LDTR_BASE, vmxGdtEntry.Base);
	__vmx_vmwrite(VMCS_GUEST_LDTR_LIMIT, vmxGdtEntry.Limit);
	__vmx_vmwrite(VMCS_GUEST_LDTR_ACCESS_RIGHTS, vmxGdtEntry.AccessRights);
	__vmx_vmwrite(VMCS_GUEST_LDTR_BASE, asm_get_ldtr() & 0xF8);

	// There is no field in the host - state area for the LDTR selector.


	ShvUtilConvertGdtEntry(reinterpret_cast<void*>(gdtBase), asm_get_tr(), &vmxGdtEntry);

	__vmx_vmwrite(VMCS_GUEST_TR_SELECTOR, vmxGdtEntry.Selector);	// GETTR() & 0xF8
	__vmx_vmwrite(VMCS_GUEST_TR_BASE, vmxGdtEntry.Base);
	__vmx_vmwrite(VMCS_GUEST_TR_LIMIT, vmxGdtEntry.Limit);
	__vmx_vmwrite(VMCS_GUEST_TR_ACCESS_RIGHTS, vmxGdtEntry.AccessRights);

	__vmx_vmwrite(VMCS_HOST_TR_SELECTOR, (asm_get_tr() & 0xF8));
	__vmx_vmwrite(VMCS_HOST_TR_BASE, vmxGdtEntry.Base);

	/// GDTR and IDTR 
	__vmx_vmwrite(VMCS_GUEST_GDTR_BASE, asm_get_gdt_base());
	__vmx_vmwrite(VMCS_GUEST_GDTR_LIMIT, asm_get_gdt_limit());
	__vmx_vmwrite(VMCS_HOST_GDTR_BASE, asm_get_gdt_base());

	__vmx_vmwrite(VMCS_GUEST_IDTR_BASE, asm_get_idt_base());
	__vmx_vmwrite(VMCS_GUEST_IDTR_LIMIT, asm_get_idt_limit());
	__vmx_vmwrite(VMCS_HOST_IDTR_BASE, asm_get_idt_base());

	/// The various MSRs as documented in the Intel SDM - Guest & Host
	if (__vmx_vmwrite(VMCS_GUEST_DEBUGCTL, __readmsr(MSR_IA32_DEBUGCTL) & 0xffffffff))	return;
	if (__vmx_vmwrite(VMCS_GUEST_DEBUGCTL_HIGH, __readmsr(MSR_IA32_DEBUGCTL) >> 32))	return;	// I don't think this is specifically needed

	if (__vmx_vmwrite(VMCS_GUEST_SYSENTER_CS, __readmsr(MSR_IA32_SYSENTER_CS)))	return;
	if (__vmx_vmwrite(VMCS_GUEST_SYSENTER_ESP, __readmsr(MSR_IA32_SYSENTER_ESP)))	return;
	if (__vmx_vmwrite(VMCS_GUEST_SYSENTER_EIP, __readmsr(MSR_IA32_SYSENTER_EIP)))	return;
	if (__vmx_vmwrite(VMCS_GUEST_EFER, __readmsr(IA32_EFER)))	return;

	if (__vmx_vmwrite(VMCS_HOST_SYSENTER_CS, __readmsr(MSR_IA32_SYSENTER_CS)))	return;
	if (__vmx_vmwrite(VMCS_HOST_SYSENTER_ESP, __readmsr(MSR_IA32_SYSENTER_ESP)))	return;
	if (__vmx_vmwrite(VMCS_HOST_SYSENTER_EIP, __readmsr(MSR_IA32_SYSENTER_EIP)))	return;

	/// VMCS link pointer
	if (__vmx_vmwrite(VMCS_GUEST_VMCS_LINK_POINTER, static_cast<size_t>(~0)))	return;

	/// VM Execution Control Fields
	/// These fields control processor behavior in VMX non-root operation.
	/// They determine in part the causes of VM exits.
	if (__vmx_vmwrite(VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS, 
		adjust_controls(pinbased_ctls.flags, IA32_VMX_PINBASED_CTLS)))	return;	// vmx_basic.vmx_controls ? IA32_VMX_TRUE_PINBASED_CTLS : IA32_VMX_PINBASED_CTLS

	if (__vmx_vmwrite(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS,
		adjust_controls(procbased_ctls.flags, IA32_VMX_PROCBASED_CTLS))) return;	// vmx_basic.vmx_controls ? IA32_VMX_TRUE_PROCBASED_CTLS : IA32_VMX_PROCBASED_CTLS

	if (__vmx_vmwrite(VMCS_CTRL_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS,
		adjust_controls(procbased_ctls2.flags, IA32_VMX_PROCBASED_CTLS2))) return;


	/// VM-exit control fields. 
	/// These fields control VM exits
	if (__vmx_vmwrite(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS,
		adjust_controls(exit_ctls.flags, IA32_VMX_EXIT_CTLS)))	return;		// vmx_basic.vmx_controls ? IA32_VMX_TRUE_EXIT_CTLS : IA32_VMX_EXIT_CTLS

	/// VM-entry control fields. 
	/// These fields control VM entries.
	if (__vmx_vmwrite(VMCS_CTRL_VMENTRY_CONTROLS,
		adjust_controls(entry_ctls.flags, IA32_VMX_ENTRY_CTLS)))	return;		// vmx_basic.vmx_controls ? IA32_VMX_TRUE_ENTRY_CTLS : IA32_VMX_ENTRY_CTLS

	/// Misc
	if (__vmx_vmwrite(VMCS_GUEST_ACTIVITY_STATE, 0))	return;	// Active State
	if (__vmx_vmwrite(VMCS_GUEST_INTERRUPTIBILITY_STATE, 0))	return;
	if (__vmx_vmwrite(VMCS_GUEST_PENDING_DEBUG_EXCEPTIONS, 0))	return;

	if (procbased_ctls.use_io_bitmaps) {
		if (__vmx_vmwrite(VMCS_CTRL_IO_BITMAP_A_ADDRESS, vcpu->vcpu_bitmaps.io_bitmap_a_physical))	return;
		if (__vmx_vmwrite(VMCS_CTRL_IO_BITMAP_B_ADDRESS, vcpu->vcpu_bitmaps.io_bitmap_b_physical))	return;
	}

	if (procbased_ctls.use_msr_bitmaps)
		if (__vmx_vmwrite(VMCS_CTRL_MSR_BITMAP_ADDRESS, vcpu->vcpu_bitmaps.msr_bitmap_physical))	return;

	if (__vmx_vmwrite(VMCS_CTRL_VIRTUAL_PROCESSOR_IDENTIFIER, KeGetCurrentProcessorNumberEx(NULL) + 1))	return;

}

/*auto hv_setup_vmcs(struct __vcpu* vcpu, void* guest_rsp) -> void {
	__descriptor64 gdtr = { 0 };
	__descriptor64 idtr = { 0 };
	__exception_bitmap exception_bitmap = { 0 };
	ia32_vmx_basic_register vmx_basic = { 0 };
	ia32_vmx_entry_ctls_register entry_controls = { 0 };
	ia32_vmx_exit_ctls_register exit_controls = { 0 };
	ia32_vmx_pinbased_ctls_register pinbased_controls = { 0 };
	ia32_vmx_procbased_ctls_register primary_controls = { 0 };
	ia32_vmx_procbased_ctls2_register secondary_controls = { 0 };

	const unsigned __int8 selector_mask = 7;

	vmx_basic.flags = __readmsr(IA32_VMX_BASIC);

	save_vmentry_fields(entry_controls);

	save_vmexit_fields(exit_controls);

	save_proc_based_fields(primary_controls);

	save_proc_secondary_fields(secondary_controls);

	set_exception_bitmap(exception_bitmap);

	save_pin_fields(pinbased_controls);

	//
	// We want to vmexit on every io and msr access
	memset(vcpu->vcpu_bitmaps.io_bitmap_a, 0xff, PAGE_SIZE);
	memset(vcpu->vcpu_bitmaps.io_bitmap_b, 0xff, PAGE_SIZE);

	memset(vcpu->vcpu_bitmaps.msr_bitmap, 0xff, PAGE_SIZE);

	//
	// Only if your upper hypervisor is vmware
	// Because Vmware tools use ports 0x5655,0x5656,0x5657,0x5658,0x5659,0x565a,0x565b,0x1090,0x1094 as I/O backdoor
	set_io_bitmap(0x5655, vcpu, false);
	set_io_bitmap(0x5656, vcpu, false);
	set_io_bitmap(0x5657, vcpu, false);
	set_io_bitmap(0x5658, vcpu, false);
	set_io_bitmap(0x5659, vcpu, false);
	set_io_bitmap(0x565a, vcpu, false);
	set_io_bitmap(0x565b, vcpu, false);
	set_io_bitmap(0x1094, vcpu, false);
	set_io_bitmap(0x1090, vcpu, false);

	__vmx_vmclear((unsigned __int64*)&vcpu->vmcs_physical);
	__vmx_vmptrld((unsigned __int64*)&vcpu->vmcs_physical);

	__sgdt(&gdtr);
	__sidt(&idtr);

	__vmx_vmwrite(VMCS_GUEST_GDTR_BASE, gdtr.base_address);
	__vmx_vmwrite(VMCS_GUEST_GDTR_LIMIT, gdtr.limit);
	__vmx_vmwrite(VMCS_HOST_GDTR_BASE, gdtr.base_address);
	__vmx_vmwrite(VMCS_GUEST_IDTR_BASE, idtr.base_address);
	__vmx_vmwrite(VMCS_GUEST_IDTR_LIMIT, idtr.limit);
	__vmx_vmwrite(VMCS_HOST_IDTR_BASE, idtr.base_address);

	/// VM Execution Control Fields
	/// These fields control processor behavior in VMX non-root operation.
	/// They determine in part the causes of VM exits.
	__vmx_vmwrite(VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS,
		adjust_controls(pinbased_controls.flags, vmx_basic.vmx_controls ? IA32_VMX_TRUE_PINBASED_CTLS : IA32_VMX_PINBASED_CTLS));

	__vmx_vmwrite(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS,
		adjust_controls(primary_controls.flags, vmx_basic.vmx_controls ? IA32_VMX_TRUE_PROCBASED_CTLS : IA32_VMX_PROCBASED_CTLS));

	__vmx_vmwrite(VMCS_CTRL_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS,
		adjust_controls(secondary_controls.flags, IA32_VMX_PROCBASED_CTLS2));


	/// VM-exit control fields. 
	/// These fields control VM exits
	__vmx_vmwrite(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS,
		adjust_controls(exit_controls.flags, vmx_basic.vmx_controls ? IA32_VMX_TRUE_EXIT_CTLS : IA32_VMX_EXIT_CTLS));

	/// VM-entry control fields. 
	/// These fields control VM entries.
	__vmx_vmwrite(VMCS_CTRL_VMENTRY_CONTROLS,
		adjust_controls(entry_controls.flags, vmx_basic.vmx_controls ? IA32_VMX_TRUE_ENTRY_CTLS : IA32_VMX_ENTRY_CTLS));

	// Segments
	fill_guest_selector_data((void*)gdtr.base_address, ES, __read_es());
	fill_guest_selector_data((void*)gdtr.base_address, CS, __read_cs());
	fill_guest_selector_data((void*)gdtr.base_address, SS, __read_ss());
	fill_guest_selector_data((void*)gdtr.base_address, DS, __read_ds());
	fill_guest_selector_data((void*)gdtr.base_address, FS, __read_fs());
	fill_guest_selector_data((void*)gdtr.base_address, GS, __read_gs());
	fill_guest_selector_data((void*)gdtr.base_address, LDTR, __read_ldtr());
	fill_guest_selector_data((void*)gdtr.base_address, TR, __read_tr());
	__vmx_vmwrite(VMCS_GUEST_FS_BASE, __readmsr(IA32_FS_BASE));
	__vmx_vmwrite(VMCS_GUEST_GS_BASE, __readmsr(IA32_GS_BASE));
	__vmx_vmwrite(VMCS_HOST_CS_SELECTOR, __read_cs() & ~selector_mask);
	__vmx_vmwrite(VMCS_HOST_SS_SELECTOR, __read_ss() & ~selector_mask);
	__vmx_vmwrite(VMCS_HOST_DS_SELECTOR, __read_ds() & ~selector_mask);
	__vmx_vmwrite(VMCS_HOST_ES_SELECTOR, __read_es() & ~selector_mask);
	__vmx_vmwrite(VMCS_HOST_FS_SELECTOR, __read_fs() & ~selector_mask);
	__vmx_vmwrite(VMCS_HOST_GS_SELECTOR, __read_gs() & ~selector_mask);
	__vmx_vmwrite(VMCS_HOST_TR_SELECTOR, __read_tr() & ~selector_mask);
	__vmx_vmwrite(VMCS_HOST_FS_BASE, __readmsr(IA32_FS_BASE));
	__vmx_vmwrite(VMCS_HOST_GS_BASE, __readmsr(IA32_GS_BASE));
	__vmx_vmwrite(VMCS_HOST_TR_BASE, get_segment_base(__read_tr(), (unsigned char*)gdtr.base_address));

	// Cr registers
	__vmx_vmwrite(VMCS_GUEST_CR0, __readcr0());
	__vmx_vmwrite(VMCS_HOST_CR0, __readcr0());
	__vmx_vmwrite(VMCS_CTRL_CR0_READ_SHADOW, __readcr0());

	__vmx_vmwrite(VMCS_GUEST_CR3, __readcr3());
	__vmx_vmwrite(VMCS_HOST_CR3, __readcr3());		// get_system_directory_table_base
	__vmx_vmwrite(VMCS_CTRL_CR3_TARGET_COUNT, 0);

	__vmx_vmwrite(VMCS_GUEST_CR4, __readcr4());
	__vmx_vmwrite(VMCS_HOST_CR4, __readcr4());
	__vmx_vmwrite(VMCS_CTRL_CR4_READ_SHADOW, __readcr4() & ~0x2000);
	__vmx_vmwrite(VMCS_CTRL_CR4_GUEST_HOST_MASK, 0x2000); // Virtual Machine Extensions Enable	

	// Debug register
	__vmx_vmwrite(VMCS_GUEST_DR7, __readdr(7));

	// RFLAGS
	__vmx_vmwrite(VMCS_GUEST_RFLAGS, __readeflags());

	// RSP and RIP
	__vmx_vmwrite(VMCS_GUEST_RSP, reinterpret_cast<size_t>(guest_rsp));
	__vmx_vmwrite(VMCS_GUEST_RIP, reinterpret_cast<size_t>(asm_restore_vmm_state));
	__vmx_vmwrite(VMCS_HOST_RSP, (unsigned __int64)vcpu->vmm_stack + VMM_STACK_SIZE);
	__vmx_vmwrite(VMCS_HOST_RIP, reinterpret_cast<size_t>(asm_host_continue_execution));

	// MSRS Guest
	__vmx_vmwrite(VMCS_GUEST_DEBUGCTL, __readmsr(IA32_DEBUGCTL));
	__vmx_vmwrite(VMCS_GUEST_SYSENTER_CS, __readmsr(IA32_SYSENTER_CS));
	__vmx_vmwrite(VMCS_GUEST_SYSENTER_ESP, __readmsr(IA32_SYSENTER_ESP));
	__vmx_vmwrite(VMCS_GUEST_SYSENTER_EIP, __readmsr(IA32_SYSENTER_EIP));
	__vmx_vmwrite(VMCS_GUEST_EFER, __readmsr(IA32_EFER));

	// MSRS Host
	__vmx_vmwrite(VMCS_HOST_SYSENTER_CS, __readmsr(IA32_SYSENTER_CS));
	__vmx_vmwrite(VMCS_HOST_SYSENTER_ESP, __readmsr(IA32_SYSENTER_ESP));
	__vmx_vmwrite(VMCS_HOST_SYSENTER_EIP, __readmsr(IA32_SYSENTER_EIP));
	__vmx_vmwrite(VMCS_HOST_EFER, __readmsr(IA32_EFER));

	// Features
	__vmx_vmwrite(VMCS_GUEST_VMCS_LINK_POINTER, ~0ULL);

	__vmx_vmwrite(VMCS_CTRL_EXCEPTION_BITMAP, exception_bitmap.all);

	if (primary_controls.use_msr_bitmaps == true)
		__vmx_vmwrite(VMCS_CTRL_MSR_BITMAP_ADDRESS, vcpu->vcpu_bitmaps.msr_bitmap_physical);

	if (primary_controls.use_io_bitmaps == true) {
		__vmx_vmwrite(VMCS_CTRL_IO_BITMAP_A_ADDRESS, vcpu->vcpu_bitmaps.io_bitmap_a_physical);
		__vmx_vmwrite(VMCS_CTRL_IO_BITMAP_B_ADDRESS, vcpu->vcpu_bitmaps.io_bitmap_b_physical);
	}

	if (secondary_controls.enable_vpid == true)
		__vmx_vmwrite(VMCS_CTRL_VIRTUAL_PROCESSOR_IDENTIFIER, KeGetCurrentProcessorNumberEx(NULL) + 1);

	if (secondary_controls.enable_ept == true && secondary_controls.enable_vpid == true)
		__vmx_vmwrite(VMCS_CTRL_EPT_POINTER, vcpu->ept_state->ept_pointer->flags);
}*/
