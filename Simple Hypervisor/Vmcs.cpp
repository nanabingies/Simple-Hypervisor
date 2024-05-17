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

auto adjust_controls(ulong Ctl, ulong Msr) -> unsigned __int64 {
	LARGE_INTEGER MsrValue = { 0 };

	MsrValue.QuadPart = __readmsr(Msr);
	Ctl &= MsrValue.HighPart;	/* bit == 0 in high word ==> must be zero */
	Ctl |= MsrValue.LowPart;	/* bit == 1 in low word  ==> must be one  */

	return Ctl;
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
	__vmx_vmwrite(VMCS_GUEST_CR0, __readcr0());
	__vmx_vmwrite(VMCS_GUEST_CR3, __readcr3());
	__vmx_vmwrite(VMCS_GUEST_CR4, __readcr4());

	__vmx_vmwrite(VMCS_HOST_CR0, __readcr0());
	__vmx_vmwrite(VMCS_HOST_CR3, __readcr3());		// Host cr3		// hv::get_system_dirbase()
	__vmx_vmwrite(VMCS_HOST_CR4, __readcr4());

	__vmx_vmwrite(VMCS_CTRL_CR0_READ_SHADOW, __readcr0());
	__vmx_vmwrite(VMCS_CTRL_CR4_READ_SHADOW, __readcr4());

	__vmx_vmwrite(VMCS_CTRL_CR3_TARGET_COUNT, __readcr3());
	__vmx_vmwrite(VMCS_CTRL_CR4_READ_SHADOW, __readcr4());
	__vmx_vmwrite(VMCS_CTRL_CR4_GUEST_HOST_MASK, 0x2000);

	/// Debug Register (DR7)
	__vmx_vmwrite(VMCS_GUEST_DR7, __readdr(7));

	/// RSP, RIP, RFLAGS - Guest & Host
	if (!__vmx_vmwrite(VMCS_GUEST_RSP, reinterpret_cast<size_t>(guest_rsp)))	return;
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
	__vmx_vmwrite(VMCS_GUEST_DEBUGCTL, __readmsr(MSR_IA32_DEBUGCTL) & 0xffffffff);
	__vmx_vmwrite(VMCS_GUEST_DEBUGCTL_HIGH, __readmsr(MSR_IA32_DEBUGCTL) >> 32);	// I don't think this is specifically needed

	__vmx_vmwrite(VMCS_GUEST_SYSENTER_CS, __readmsr(MSR_IA32_SYSENTER_CS));
	__vmx_vmwrite(VMCS_GUEST_SYSENTER_ESP, __readmsr(MSR_IA32_SYSENTER_ESP));
	__vmx_vmwrite(VMCS_GUEST_SYSENTER_EIP, __readmsr(MSR_IA32_SYSENTER_EIP));
	__vmx_vmwrite(VMCS_GUEST_EFER, __readmsr(IA32_EFER));

	__vmx_vmwrite(VMCS_HOST_SYSENTER_CS, __readmsr(MSR_IA32_SYSENTER_CS));
	__vmx_vmwrite(VMCS_HOST_SYSENTER_ESP, __readmsr(MSR_IA32_SYSENTER_ESP));
	__vmx_vmwrite(VMCS_HOST_SYSENTER_EIP, __readmsr(MSR_IA32_SYSENTER_EIP));

	/// VMCS link pointer
	__vmx_vmwrite(VMCS_GUEST_VMCS_LINK_POINTER, static_cast<size_t>(~0));

	/// VM Execution Control Fields
	/// These fields control processor behavior in VMX non-root operation.
	/// They determine in part the causes of VM exits.
	if (__vmx_vmwrite(VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS, 
		adjust_controls(pinbased_ctls.flags, vmx_basic.vmx_controls ? IA32_VMX_TRUE_PINBASED_CTLS : IA32_VMX_PINBASED_CTLS)))	return;

	if (__vmx_vmwrite(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS,
		adjust_controls(procbased_ctls.flags, vmx_basic.vmx_controls ? IA32_VMX_TRUE_PROCBASED_CTLS : IA32_VMX_PROCBASED_CTLS))) return;

	if (__vmx_vmwrite(VMCS_CTRL_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS,
		adjust_controls(procbased_ctls2.flags, IA32_VMX_PROCBASED_CTLS2))) return;


	/// VM-exit control fields. 
	/// These fields control VM exits
	if (__vmx_vmwrite(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS,
		adjust_controls(exit_ctls.flags, vmx_basic.vmx_controls ? IA32_VMX_TRUE_EXIT_CTLS : IA32_VMX_EXIT_CTLS)))	return;

	/// VM-entry control fields. 
	/// These fields control VM entries.
	if (__vmx_vmwrite(VMCS_CTRL_VMENTRY_CONTROLS,
		adjust_controls(entry_ctls.flags, vmx_basic.vmx_controls ? IA32_VMX_TRUE_ENTRY_CTLS : IA32_VMX_ENTRY_CTLS)))	return;

	/// Misc
	if (__vmx_vmwrite(VMCS_GUEST_ACTIVITY_STATE, 0))	return;	// Active State
	if (__vmx_vmwrite(VMCS_GUEST_INTERRUPTIBILITY_STATE, 0))	return;
	if (__vmx_vmwrite(VMCS_GUEST_PENDING_DEBUG_EXCEPTIONS, 0))	return;

	if (__vmx_vmwrite(VMCS_CTRL_IO_BITMAP_A_ADDRESS, vcpu->vcpu_bitmaps.io_bitmap_a_physical))	return;
	if (__vmx_vmwrite(VMCS_CTRL_IO_BITMAP_B_ADDRESS, vcpu->vcpu_bitmaps.io_bitmap_b_physical))	return;

	if (__vmx_vmwrite(VMCS_CTRL_MSR_BITMAP_ADDRESS, vcpu->vcpu_bitmaps.msr_bitmap_physical))	return;

	if (__vmx_vmwrite(VMCS_CTRL_VIRTUAL_PROCESSOR_IDENTIFIER, KeGetCurrentProcessorNumberEx(NULL) + 1))	return;

}
