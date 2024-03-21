#include "stdafx.h"
#define VMCS_CTRL_TSC_OFFSET_HIGH	0x2011

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

auto AdjustControls(ulong Ctl, ulong Msr) -> unsigned __int64 {
	LARGE_INTEGER MsrValue = { 0 };

	MsrValue.QuadPart = __readmsr(Msr);
	Ctl &= MsrValue.HighPart;	/* bit == 0 in high word ==> must be zero */
	Ctl |= MsrValue.LowPart;	/* bit == 1 in low word  ==> must be one  */

	return Ctl;
}

auto setup_vmcs(unsigned long processor_number, void* guest_rsp, uint64_t cr3) -> EVmErrors {
	PAGED_CODE();
	ret_val = 0;

	//
	// Control Registers - Guest & Host
	//
	if (__vmx_vmwrite(VMCS_GUEST_CR0, __readcr0()) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_CR3, __readcr3()) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_CR4, __readcr4()) != 0)	return VM_ERROR_ERR_INFO_ERR;

	if (__vmx_vmwrite(VMCS_HOST_CR0, __readcr0()) != 0)		return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_HOST_CR3, static_cast<size_t>(cr3)) != 0)		return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_HOST_CR4, __readcr4()) != 0)		return VM_ERROR_ERR_INFO_ERR;

	if (__vmx_vmwrite(VMCS_CTRL_CR0_READ_SHADOW, __readcr0()) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_CTRL_CR4_READ_SHADOW, __readcr4()) != 0)	return VM_ERROR_ERR_INFO_ERR;

	__vmx_vmwrite(VMCS_CTRL_CR4_READ_SHADOW, __readcr4());

	//
	// Debug Register (DR7)
	//
	if (__vmx_vmwrite(VMCS_GUEST_DR7, __readdr(7)) != 0)	return VM_ERROR_ERR_INFO_ERR;

	//
	// RSP, RIP, RFLAGS - Guest & Host
	//
	vmm_context[processor_number].guest_rip = reinterpret_cast<size_t>(guest_rsp); //AsmGuestContinueExecution;
	vmm_context[processor_number].guest_rsp = reinterpret_cast<size_t>(guest_rsp);

	vmm_context[processor_number].host_rip = (unsigned __int64)asm_host_continue_execution;
	vmm_context[processor_number].host_rsp = //vmm_context[processor_number].host_stack;
		(static_cast<size_t>(vmm_context[processor_number].host_stack) + 
			HOST_STACK_SIZE - sizeof(void*) - sizeof(struct guest_registers));

	if (__vmx_vmwrite(VMCS_GUEST_RSP, static_cast<size_t>(vmm_context[processor_number].guest_rsp)) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_RIP, static_cast<size_t>(vmm_context[processor_number].guest_rip) != 0))	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_RFLAGS, __readeflags()) != 0)	return VM_ERROR_ERR_INFO_ERR;

	if (__vmx_vmwrite(VMCS_HOST_RSP, static_cast<size_t>(vmm_context[processor_number].host_rsp)) != 0)		return VM_ERROR_ERR_INFO_ERR;
	// Address host should point to, to kick things off when vmexit occurs
	if (__vmx_vmwrite(VMCS_HOST_RIP, static_cast<size_t>(vmm_context[processor_number].host_rip)) != 0)		return VM_ERROR_ERR_INFO_ERR;


	//
	// CS, SS, DS, ES, FS, GS, LDTR, and TR -- Guest & Host
	//
	CONTEXT ctx;
	RtlCaptureContext(&ctx);

	uint64_t gdtBase = asm_get_gdt_base();
	vmx_gdtentry64 vmxGdtEntry;

	ShvUtilConvertGdtEntry(reinterpret_cast<void*>(gdtBase), ctx.SegCs, &vmxGdtEntry);

	if (__vmx_vmwrite(VMCS_GUEST_CS_SELECTOR, vmxGdtEntry.Selector) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_CS_BASE, vmxGdtEntry.Base) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_CS_LIMIT, vmxGdtEntry.Limit) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_CS_ACCESS_RIGHTS, vmxGdtEntry.AccessRights) != 0)	return VM_ERROR_ERR_INFO_ERR;

	if (__vmx_vmwrite(VMCS_HOST_CS_SELECTOR, (ctx.SegCs & 0xF8)) != 0)	return VM_ERROR_ERR_INFO_ERR;

	ShvUtilConvertGdtEntry(reinterpret_cast<void*>(gdtBase), ctx.SegSs, &vmxGdtEntry);

	if (__vmx_vmwrite(VMCS_GUEST_SS_SELECTOR, vmxGdtEntry.Selector) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_SS_BASE, vmxGdtEntry.Base) != 0)		return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_SS_LIMIT, vmxGdtEntry.Limit) != 0)		return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_SS_ACCESS_RIGHTS, vmxGdtEntry.AccessRights) != 0)	return VM_ERROR_ERR_INFO_ERR;

	if (__vmx_vmwrite(VMCS_HOST_SS_SELECTOR, (ctx.SegSs & 0xF8)) != 0)	return VM_ERROR_ERR_INFO_ERR;

	ShvUtilConvertGdtEntry(reinterpret_cast<void*>(gdtBase), ctx.SegDs, &vmxGdtEntry);

	if (__vmx_vmwrite(VMCS_GUEST_DS_SELECTOR, vmxGdtEntry.Selector) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_DS_BASE, vmxGdtEntry.Base) != 0)		return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_DS_LIMIT, vmxGdtEntry.Limit) != 0)		return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_DS_ACCESS_RIGHTS, vmxGdtEntry.AccessRights) != 0)	return VM_ERROR_ERR_INFO_ERR;

	if (__vmx_vmwrite(VMCS_HOST_DS_SELECTOR, (ctx.SegDs & 0xF8)) != 0)	return VM_ERROR_ERR_INFO_ERR;

	ShvUtilConvertGdtEntry(reinterpret_cast<void*>(gdtBase), ctx.SegEs, &vmxGdtEntry);

	if (__vmx_vmwrite(VMCS_GUEST_ES_SELECTOR, vmxGdtEntry.Selector) != 0)	return VM_ERROR_ERR_INFO_ERR;	// GETES() & 0xF8
	if (__vmx_vmwrite(VMCS_GUEST_ES_BASE, vmxGdtEntry.Base) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_ES_LIMIT, vmxGdtEntry.Limit) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_ES_ACCESS_RIGHTS, vmxGdtEntry.AccessRights) != 0)	return VM_ERROR_ERR_INFO_ERR;

	if (__vmx_vmwrite(VMCS_HOST_ES_SELECTOR, (ctx.SegEs & 0xF8)) != 0)	return VM_ERROR_ERR_INFO_ERR;

	ShvUtilConvertGdtEntry(reinterpret_cast<void*>(gdtBase), ctx.SegFs, &vmxGdtEntry);

	if (__vmx_vmwrite(VMCS_GUEST_FS_SELECTOR, vmxGdtEntry.Selector) != 0) return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_FS_BASE, __readmsr(IA32_FS_BASE)) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_FS_LIMIT, vmxGdtEntry.Limit) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_FS_ACCESS_RIGHTS, vmxGdtEntry.AccessRights) != 0)	return VM_ERROR_ERR_INFO_ERR;

	if (__vmx_vmwrite(VMCS_HOST_FS_SELECTOR, (ctx.SegFs & 0xF8)) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_HOST_FS_BASE, __readmsr(IA32_FS_BASE)) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_FS_BASE, __readmsr(IA32_FS_BASE)) != 0)	return VM_ERROR_ERR_INFO_ERR;

	ShvUtilConvertGdtEntry(reinterpret_cast<void*>(gdtBase), ctx.SegGs, &vmxGdtEntry);

	if (__vmx_vmwrite(VMCS_GUEST_GS_SELECTOR, vmxGdtEntry.Selector) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_GS_BASE, __readmsr(IA32_GS_BASE)) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_GS_LIMIT, vmxGdtEntry.Limit) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_GS_ACCESS_RIGHTS, vmxGdtEntry.AccessRights) != 0)	return VM_ERROR_ERR_INFO_ERR;

	if (__vmx_vmwrite(VMCS_HOST_GS_SELECTOR, (ctx.SegGs & 0xF8)) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_HOST_GS_BASE, __readmsr(IA32_GS_BASE)) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_GS_BASE, __readmsr(IA32_GS_BASE)) != 0)	return VM_ERROR_ERR_INFO_ERR;


	ShvUtilConvertGdtEntry(reinterpret_cast<void*>(gdtBase), asm_get_ldtr(), &vmxGdtEntry);

	if (__vmx_vmwrite(VMCS_GUEST_LDTR_SELECTOR, vmxGdtEntry.Selector) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_LDTR_BASE, vmxGdtEntry.Base) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_LDTR_LIMIT, vmxGdtEntry.Limit) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_LDTR_ACCESS_RIGHTS, vmxGdtEntry.AccessRights) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_LDTR_BASE, asm_get_ldtr() & 0xF8) != 0)	return VM_ERROR_ERR_INFO_ERR;

	// There is no field in the host - state area for the LDTR selector.


	ShvUtilConvertGdtEntry(reinterpret_cast<void*>(gdtBase), asm_get_tr(), &vmxGdtEntry);

	if (__vmx_vmwrite(VMCS_GUEST_TR_SELECTOR, vmxGdtEntry.Selector) != 0)	return VM_ERROR_ERR_INFO_ERR;	// GETTR() & 0xF8
	if (__vmx_vmwrite(VMCS_GUEST_TR_BASE, vmxGdtEntry.Base) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_TR_LIMIT, vmxGdtEntry.Limit) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_TR_ACCESS_RIGHTS, vmxGdtEntry.AccessRights) != 0)	return VM_ERROR_ERR_INFO_ERR;

	if (__vmx_vmwrite(VMCS_HOST_TR_SELECTOR, (asm_get_tr() & 0xF8)) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_HOST_TR_BASE, vmxGdtEntry.Base) != 0)	return VM_ERROR_ERR_INFO_ERR;

	//
	// GDTR and IDTR 
	//

	if (__vmx_vmwrite(VMCS_GUEST_GDTR_BASE, asm_get_gdt_base()) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_GDTR_LIMIT, asm_get_gdt_limit()) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_HOST_GDTR_BASE, asm_get_gdt_base()) != 0)	return VM_ERROR_ERR_INFO_ERR;

	if (__vmx_vmwrite(VMCS_GUEST_IDTR_BASE, asm_get_idt_base()) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_IDTR_LIMIT, asm_get_idt_limit()) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_HOST_IDTR_BASE, asm_get_idt_base()) != 0)	return VM_ERROR_ERR_INFO_ERR;

	//
	// The various MSRs as documented in the Intel SDM - Guest & Host
	//
	if (__vmx_vmwrite(VMCS_GUEST_DEBUGCTL, __readmsr(MSR_IA32_DEBUGCTL) & 0xffffffff) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_DEBUGCTL_HIGH, __readmsr(MSR_IA32_DEBUGCTL) >> 32) != 0)	return VM_ERROR_ERR_INFO_ERR;	// I don't think this is specifically needed

	if (__vmx_vmwrite(VMCS_GUEST_SYSENTER_CS, __readmsr(MSR_IA32_SYSENTER_CS)) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_SYSENTER_ESP, __readmsr(MSR_IA32_SYSENTER_ESP)) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_SYSENTER_EIP, __readmsr(MSR_IA32_SYSENTER_EIP)) != 0)	return VM_ERROR_ERR_INFO_ERR;

	if (__vmx_vmwrite(VMCS_HOST_SYSENTER_CS, __readmsr(MSR_IA32_SYSENTER_CS)) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_HOST_SYSENTER_ESP, __readmsr(MSR_IA32_SYSENTER_ESP)) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_HOST_SYSENTER_EIP, __readmsr(MSR_IA32_SYSENTER_EIP)) != 0)	return VM_ERROR_ERR_INFO_ERR;

	//
	// SMBASE (32 bits)
	// This register contains the base address of the logical processor's SMRAM image.
	// 
	//__vmx_vmwrite(VMCS_GUEST_SMBASE, );

	//
	// VMCS link pointer
	//
	if (__vmx_vmwrite(VMCS_GUEST_VMCS_LINK_POINTER, static_cast<size_t>(~0)) != 0)	return VM_ERROR_ERR_INFO_ERR;

	//
	// VM Execution Control Fields
	// These fields control processor behavior in VMX non-root operation.
	// They determine in part the causes of VM exits.
	//
	if (__vmx_vmwrite(VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS, AdjustControls(0, IA32_VMX_PINBASED_CTLS)) != 0)	return VM_ERROR_ERR_INFO_ERR;

	if (__vmx_vmwrite(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS,
		AdjustControls(IA32_VMX_PROCBASED_CTLS_HLT_EXITING_FLAG | IA32_VMX_PROCBASED_CTLS_USE_MSR_BITMAPS_FLAG |
			IA32_VMX_PROCBASED_CTLS_CR3_LOAD_EXITING_FLAG | /*IA32_VMX_PROCBASED_CTLS_CR3_STORE_EXITING_FLAG |*/
			IA32_VMX_PROCBASED_CTLS_USE_IO_BITMAPS_FLAG |
			IA32_VMX_PROCBASED_CTLS_ACTIVATE_SECONDARY_CONTROLS_FLAG, IA32_VMX_PROCBASED_CTLS)) != 0)	return VM_ERROR_ERR_INFO_ERR;

	if (__vmx_vmwrite(VMCS_CTRL_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS,
		AdjustControls(IA32_VMX_PROCBASED_CTLS2_ENABLE_XSAVES_FLAG | IA32_VMX_PROCBASED_CTLS2_ENABLE_RDTSCP_FLAG |
			IA32_VMX_PROCBASED_CTLS2_ENABLE_EPT_FLAG | IA32_VMX_PROCBASED_CTLS2_DESCRIPTOR_TABLE_EXITING_FLAG |
			IA32_VMX_PROCBASED_CTLS2_ENABLE_VPID_FLAG | IA32_VMX_PROCBASED_CTLS2_ENABLE_INVPCID_FLAG,
			IA32_VMX_PROCBASED_CTLS2)) != 0)	return VM_ERROR_ERR_INFO_ERR;


	//
	// VM-exit control fields. 
	// These fields control VM exits
	//
	if (__vmx_vmwrite(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS,
		AdjustControls(IA32_VMX_EXIT_CTLS_HOST_ADDRESS_SPACE_SIZE_FLAG | IA32_VMX_EXIT_CTLS_ACKNOWLEDGE_INTERRUPT_ON_EXIT_FLAG,
			IA32_VMX_EXIT_CTLS)) != 0)	return VM_ERROR_ERR_INFO_ERR;

	//
	// VM-entry control fields. 
	// These fields control VM entries.
	//
	if (__vmx_vmwrite(VMCS_CTRL_VMENTRY_CONTROLS,
		AdjustControls(IA32_VMX_ENTRY_CTLS_IA32E_MODE_GUEST_FLAG, IA32_VMX_ENTRY_CTLS)) != 0)	return VM_ERROR_ERR_INFO_ERR;

	//
	// VM-exit information fields. 
	// These fields receive information on VM exits and describe the cause and the nature of VM exits.
	//
	//if (__vmx_vmwrite(VMCS_VMEXIT_INSTRUCTION_INFO,
	//	AdjustControls(IA32_VMX_EXIT_CTLS_HOST_ADDRESS_SPACE_SIZE_FLAG, IA32_VMX_EXIT_CTLS)) != 0)	return VM_ERROR_ERR_INFO_ERR;

	//__debugbreak();

	//
	// Misc
	//
	if (__vmx_vmwrite(VMCS_GUEST_ACTIVITY_STATE, 0) != 0)	return VM_ERROR_ERR_INFO_ERR;	// Active State
	if (__vmx_vmwrite(VMCS_GUEST_INTERRUPTIBILITY_STATE, 0) != 0)	return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_GUEST_PENDING_DEBUG_EXCEPTIONS, 0) != 0)	return VM_ERROR_ERR_INFO_ERR;

	if (__vmx_vmwrite(VMCS_CTRL_IO_BITMAP_A_ADDRESS, (size_t)vmm_context[processor_number].io_bitmap_a_phys_addr) != 0) return VM_ERROR_ERR_INFO_ERR;
	if (__vmx_vmwrite(VMCS_CTRL_IO_BITMAP_B_ADDRESS, (size_t)vmm_context[processor_number].io_bitmap_b_phys_addr) != 0)	return VM_ERROR_ERR_INFO_ERR;

	if (__vmx_vmwrite(VMCS_CTRL_MSR_BITMAP_ADDRESS, vmm_context[processor_number].msr_bitmap_phys_addr) != 0)	return VM_ERROR_ERR_INFO_ERR;

	//__vmx_vmwrite(VMCS_CTRL_EPT_POINTER, vmm_context[processor_number].EptPtr);
	if (__vmx_vmwrite(VMCS_CTRL_VIRTUAL_PROCESSOR_IDENTIFIER, KeGetCurrentProcessorNumberEx(NULL) + 1) != 0)	return VM_ERROR_ERR_INFO_ERR;

	ia32_vmx_misc_register misc;
	misc.flags = __readmsr(IA32_VMX_MISC);
	
	if (__vmx_vmwrite(VMCS_CTRL_CR3_TARGET_COUNT, misc.cr3_target_count) != 0)	return VM_ERROR_ERR_INFO_ERR;

	for (unsigned iter = 0; iter < misc.cr3_target_count; iter++) {
		if (__vmx_vmwrite(VMCS_CTRL_CR3_TARGET_VALUE_0 + (iter * 2), 0) != 0)	return VM_ERROR_ERR_INFO_ERR;

	}

	return VM_ERROR_OK;

}
