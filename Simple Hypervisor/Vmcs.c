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
	_Out_ PVMX_GDTENTRY64 VmxGdtEntry
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

ULONG AdjustControls(ULONG Ctl, ULONG Msr) {
	LARGE_INTEGER MsrValue = { 0 };

	MsrValue.QuadPart = __readmsr(Msr);
	Ctl &= MsrValue.HighPart;	/* bit == 0 in high word ==> must be zero */
	Ctl |= MsrValue.LowPart;	/* bit == 1 in low word  ==> must be one  */

	return Ctl;
}

EVmErrors SetupVmcs(ULONG processorNumber) {

	//
	// Control Registers - Guest & Host
	//
	__vmx_vmwrite(VMCS_GUEST_CR0, __readcr0());
	__vmx_vmwrite(VMCS_GUEST_CR3, __readcr3());
	__vmx_vmwrite(VMCS_GUEST_CR4, __readcr4());

	__vmx_vmwrite(VMCS_HOST_CR0, __readcr0());
	__vmx_vmwrite(VMCS_HOST_CR3, __readcr3());
	__vmx_vmwrite(VMCS_HOST_CR4, __readcr4());

	__vmx_vmwrite(VMCS_CTRL_CR0_READ_SHADOW, __readcr0());
	__vmx_vmwrite(VMCS_CTRL_CR4_READ_SHADOW, __readcr4());

	//
	// Debug Register (DR7)
	//
	__vmx_vmwrite(VMCS_GUEST_DR7, __readdr(7));

	//
	// RSP, RIP, RFLAGS - Guest & Host
	//
	__vmx_vmwrite(VMCS_GUEST_RSP, (size_t)vmm_context[processorNumber].GuestMemory);	// g_GuestMemory
	__vmx_vmwrite(VMCS_GUEST_RIP, (size_t)GuestContinueExecution);						// g_GuestMemory
	__vmx_vmwrite(VMCS_GUEST_RFLAGS, __readeflags());

	__vmx_vmwrite(VMCS_HOST_RSP, ((size_t)vmm_context[processorNumber].HostStack + STACK_SIZE - 1));
	// Address host should point to, to kick things off when vmexit occurs
	__vmx_vmwrite(VMCS_HOST_RIP, (size_t)HostContinueExecution); 


	//
	// CS, SS, DS, ES, FS, GS, LDTR, and TR -- Guest & Host
	//
	CONTEXT ctx;
	RtlCaptureContext(&ctx);

	UINT64 gdtBase = GetGdtBase();
	VMX_GDTENTRY64 vmxGdtEntry;

	ShvUtilConvertGdtEntry((void*)gdtBase, ctx.SegCs, &vmxGdtEntry);

	__vmx_vmwrite(VMCS_GUEST_CS_SELECTOR, vmxGdtEntry.Selector);
	__vmx_vmwrite(VMCS_GUEST_CS_BASE, vmxGdtEntry.Base);
	__vmx_vmwrite(VMCS_GUEST_CS_LIMIT, vmxGdtEntry.Limit);
	__vmx_vmwrite(VMCS_GUEST_CS_ACCESS_RIGHTS, vmxGdtEntry.AccessRights);

	__vmx_vmwrite(VMCS_HOST_CS_SELECTOR, (ctx.SegCs & 0xF8));

	ShvUtilConvertGdtEntry((void*)gdtBase, ctx.SegSs, &vmxGdtEntry);

	__vmx_vmwrite(VMCS_GUEST_SS_SELECTOR, vmxGdtEntry.Selector);
	__vmx_vmwrite(VMCS_GUEST_SS_BASE, vmxGdtEntry.Base);
	__vmx_vmwrite(VMCS_GUEST_SS_LIMIT, vmxGdtEntry.Limit);
	__vmx_vmwrite(VMCS_GUEST_SS_ACCESS_RIGHTS, vmxGdtEntry.AccessRights);

	__vmx_vmwrite(VMCS_HOST_SS_SELECTOR, (ctx.SegSs & 0xF8));

	ShvUtilConvertGdtEntry((void*)gdtBase, ctx.SegDs, &vmxGdtEntry);

	__vmx_vmwrite(VMCS_GUEST_DS_SELECTOR, vmxGdtEntry.Selector);
	__vmx_vmwrite(VMCS_GUEST_DS_BASE, vmxGdtEntry.Base);
	__vmx_vmwrite(VMCS_GUEST_DS_LIMIT, vmxGdtEntry.Limit);
	__vmx_vmwrite(VMCS_GUEST_DS_ACCESS_RIGHTS, vmxGdtEntry.AccessRights);

	__vmx_vmwrite(VMCS_HOST_DS_SELECTOR, (ctx.SegDs & 0xF8));

	ShvUtilConvertGdtEntry((void*)gdtBase, ctx.SegEs, &vmxGdtEntry);

	__vmx_vmwrite(VMCS_GUEST_ES_SELECTOR, vmxGdtEntry.Selector);	// GETES() & 0xF8
	__vmx_vmwrite(VMCS_GUEST_ES_BASE, vmxGdtEntry.Base);
	__vmx_vmwrite(VMCS_GUEST_ES_LIMIT, vmxGdtEntry.Limit);
	__vmx_vmwrite(VMCS_GUEST_ES_ACCESS_RIGHTS, vmxGdtEntry.AccessRights);

	__vmx_vmwrite(VMCS_HOST_ES_SELECTOR, (ctx.SegEs & 0xF8));

	ShvUtilConvertGdtEntry((void*)gdtBase, ctx.SegFs, &vmxGdtEntry);

	__vmx_vmwrite(VMCS_GUEST_FS_SELECTOR, vmxGdtEntry.Selector);
	__vmx_vmwrite(VMCS_GUEST_FS_BASE, __readmsr(IA32_FS_BASE));
	__vmx_vmwrite(VMCS_GUEST_FS_LIMIT, vmxGdtEntry.Limit);
	__vmx_vmwrite(VMCS_GUEST_FS_ACCESS_RIGHTS, vmxGdtEntry.AccessRights);

	__vmx_vmwrite(VMCS_HOST_FS_SELECTOR, (ctx.SegFs & 0xF8));
	__vmx_vmwrite(VMCS_HOST_FS_BASE, __readmsr(IA32_FS_BASE));
	__vmx_vmwrite(VMCS_GUEST_FS_BASE, __readmsr(IA32_FS_BASE));

	ShvUtilConvertGdtEntry((void*)gdtBase, ctx.SegGs, &vmxGdtEntry);

	__vmx_vmwrite(VMCS_GUEST_GS_SELECTOR, vmxGdtEntry.Selector);
	__vmx_vmwrite(VMCS_GUEST_GS_BASE, __readmsr(IA32_GS_BASE));
	__vmx_vmwrite(VMCS_GUEST_GS_LIMIT, vmxGdtEntry.Limit);
	__vmx_vmwrite(VMCS_GUEST_GS_ACCESS_RIGHTS, vmxGdtEntry.AccessRights);

	__vmx_vmwrite(VMCS_HOST_GS_SELECTOR, (ctx.SegGs & 0xF8));
	__vmx_vmwrite(VMCS_HOST_GS_BASE, __readmsr(IA32_GS_BASE));
	__vmx_vmwrite(VMCS_GUEST_GS_BASE, __readmsr(IA32_GS_BASE));


	ShvUtilConvertGdtEntry((void*)gdtBase, GetLdtr(), &vmxGdtEntry);

	__vmx_vmwrite(VMCS_GUEST_LDTR_SELECTOR, vmxGdtEntry.Selector);
	__vmx_vmwrite(VMCS_GUEST_LDTR_BASE, vmxGdtEntry.Base);
	__vmx_vmwrite(VMCS_GUEST_LDTR_LIMIT, vmxGdtEntry.Limit);
	__vmx_vmwrite(VMCS_GUEST_LDTR_ACCESS_RIGHTS, vmxGdtEntry.AccessRights);
	__vmx_vmwrite(VMCS_GUEST_LDTR_BASE, GetLdtr() & 0xF8);

	// There is no field in the host - state area for the LDTR selector.


	ShvUtilConvertGdtEntry((void*)gdtBase, GetTr(), &vmxGdtEntry);

	__vmx_vmwrite(VMCS_GUEST_TR_SELECTOR, vmxGdtEntry.Selector);	// GETTR() & 0xF8
	__vmx_vmwrite(VMCS_GUEST_TR_BASE, vmxGdtEntry.Base);
	__vmx_vmwrite(VMCS_GUEST_TR_LIMIT, vmxGdtEntry.Limit);
	__vmx_vmwrite(VMCS_GUEST_TR_ACCESS_RIGHTS, vmxGdtEntry.AccessRights);

	__vmx_vmwrite(VMCS_HOST_TR_SELECTOR, (GetTr() & 0xF8));
	__vmx_vmwrite(VMCS_HOST_TR_BASE, vmxGdtEntry.Base);

	//
	// GDTR and IDTR 
	//

	__vmx_vmwrite(VMCS_GUEST_GDTR_BASE, GetGdtBase());
	__vmx_vmwrite(VMCS_GUEST_GDTR_LIMIT, GetGdtLimit());
	__vmx_vmwrite(VMCS_HOST_GDTR_BASE, GetGdtBase());

	__vmx_vmwrite(VMCS_GUEST_IDTR_BASE, GetIdtBase());
	__vmx_vmwrite(VMCS_GUEST_IDTR_LIMIT, GetIdtLimit());
	__vmx_vmwrite(VMCS_HOST_IDTR_BASE, GetIdtBase());

	//
	// The various MSRs as documented in the Intel SDM - Guest & Host
	//
	__vmx_vmwrite(VMCS_GUEST_DEBUGCTL, __readmsr(MSR_IA32_DEBUGCTL) & 0xffffffff);
	__vmx_vmwrite(VMCS_GUEST_DEBUGCTL_HIGH, __readmsr(MSR_IA32_DEBUGCTL) >> 32);	// I don't think this is specifically needed

	__vmx_vmwrite(VMCS_GUEST_SYSENTER_CS, __readmsr(MSR_IA32_SYSENTER_CS));
	__vmx_vmwrite(VMCS_GUEST_SYSENTER_ESP, __readmsr(MSR_IA32_SYSENTER_ESP));
	__vmx_vmwrite(VMCS_GUEST_SYSENTER_EIP, __readmsr(MSR_IA32_SYSENTER_EIP));

	__vmx_vmwrite(VMCS_HOST_SYSENTER_CS, __readmsr(MSR_IA32_SYSENTER_CS));
	__vmx_vmwrite(VMCS_HOST_SYSENTER_ESP, __readmsr(MSR_IA32_SYSENTER_ESP));
	__vmx_vmwrite(VMCS_HOST_SYSENTER_EIP, __readmsr(MSR_IA32_SYSENTER_EIP));

	//
	// SMBASE (32 bits)
	// This register contains the base address of the logical processor's SMRAM image.
	// 
	//__vmx_vmwrite(VMCS_GUEST_SMBASE, );

	//
	// VMCS link pointer
	//
	__vmx_vmwrite(VMCS_GUEST_VMCS_LINK_POINTER, (size_t)(~0));

	//
	// VM Execution Control Fields
	// These fields control processor behavior in VMX non-root operation.
	// They determine in part the causes of VM exits.
	//
	__vmx_vmwrite(VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS, AdjustControls(0, IA32_VMX_PINBASED_CTLS));

	__vmx_vmwrite(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS,
		AdjustControls(IA32_VMX_PROCBASED_CTLS_HLT_EXITING_FLAG | IA32_VMX_PROCBASED_CTLS_USE_MSR_BITMAPS_FLAG |
			IA32_VMX_PROCBASED_CTLS_CR3_LOAD_EXITING_FLAG | IA32_VMX_PROCBASED_CTLS_CR3_STORE_EXITING_FLAG |
			IA32_VMX_PROCBASED_CTLS_USE_IO_BITMAPS_FLAG |
			IA32_VMX_PROCBASED_CTLS_ACTIVATE_SECONDARY_CONTROLS_FLAG, IA32_VMX_PROCBASED_CTLS));

	__vmx_vmwrite(VMCS_CTRL_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS,
		AdjustControls(IA32_VMX_PROCBASED_CTLS2_ENABLE_XSAVES_FLAG | IA32_VMX_PROCBASED_CTLS2_ENABLE_RDTSCP_FLAG,
			IA32_VMX_PROCBASED_CTLS2));

	//
	// VM-exit control fields. 
	// These fields control VM exits
	//
	__vmx_vmwrite(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS,
		AdjustControls(IA32_VMX_EXIT_CTLS_HOST_ADDRESS_SPACE_SIZE_FLAG | IA32_VMX_EXIT_CTLS_ACKNOWLEDGE_INTERRUPT_ON_EXIT_FLAG,
			IA32_VMX_EXIT_CTLS));

	//
	// VM-entry control fields. 
	// These fields control VM entries.
	//
	__vmx_vmwrite(VMCS_CTRL_VMENTRY_CONTROLS,
		AdjustControls(IA32_VMX_ENTRY_CTLS_IA32E_MODE_GUEST_FLAG, IA32_VMX_ENTRY_CTLS));

	//
	// VM-exit information fields. 
	// These fields receive information on VM exits and describe the cause and the nature of VM exits.
	//
	//__vmx_vmwrite(VMCS_VMEXIT_INSTRUCTION_INFO, AdjustControls(, IA32_VMX_EXIT_CTLS));

	//
	// Misc
	//
	__vmx_vmwrite(VMCS_GUEST_ACTIVITY_STATE, 0);	// Active State
	__vmx_vmwrite(VMCS_GUEST_INTERRUPTIBILITY_STATE, 0);
	__vmx_vmwrite(VMCS_GUEST_PENDING_DEBUG_EXCEPTIONS, 0);

	__vmx_vmwrite(VMCS_CTRL_IO_BITMAP_A_ADDRESS, vmm_context[processorNumber].ioBitmapAPhys);
	__vmx_vmwrite(VMCS_CTRL_IO_BITMAP_B_ADDRESS, vmm_context[processorNumber].ioBitmapBPhys);

	__vmx_vmwrite(VMCS_CTRL_MSR_BITMAP_ADDRESS, vmm_context[processorNumber].msrBitmapPhys);
	
	IA32_VMX_MISC_REGISTER misc;
	misc.AsUInt = __readmsr(IA32_VMX_MISC);
	__vmx_vmwrite(VMCS_CTRL_CR3_TARGET_COUNT, misc.Cr3TargetCount);

	for (unsigned iter = 0; iter < misc.Cr3TargetCount; iter++) {
		__vmx_vmwrite(VMCS_CTRL_CR3_TARGET_VALUE_0 + (iter * 2), 0);
	
	}

	
	return VM_ERROR_OK;
}