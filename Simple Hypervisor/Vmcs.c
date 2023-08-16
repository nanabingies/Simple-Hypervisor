#include "stdafx.h"
#define VMCS_CTRL_TSC_OFFSET_HIGH	0x2011

BOOLEAN GetSegmentDescriptor(PVMCS_SEGMENT_SELECTOR SegmentSelector, USHORT Selector, PUCHAR GdtBase) {
	PSEGMENT_DESCRIPTOR SegDesc;

	if (!SegmentSelector)
		return FALSE;

	if (Selector & 0x4)
	{
		return FALSE;
	}

	SegDesc = (PSEGMENT_DESCRIPTOR)((PUCHAR)GdtBase + (Selector & ~0x7));

	SegmentSelector->SEL = Selector;
	SegmentSelector->BASE = SegDesc->BASE0 | SegDesc->BASE1 << 16 | SegDesc->BASE2 << 24;
	SegmentSelector->LIMIT = SegDesc->LIMIT0 | (SegDesc->LIMIT1ATTR1 & 0xf) << 16;
	SegmentSelector->ATTRIBUTES.UCHARs = SegDesc->ATTR0 | (SegDesc->LIMIT1ATTR1 & 0xf0) << 4;

	if (!(SegDesc->ATTR0 & 0x10))
	{ // LA_ACCESSED
		ULONG64 Tmp;
		// this is a TSS or callgate etc, save the base high part
		Tmp = (*(PULONG64)((PUCHAR)SegDesc + 8));
		SegmentSelector->BASE = (SegmentSelector->BASE & 0xffffffff) | (Tmp << 32);
	}

	if (SegmentSelector->ATTRIBUTES.Fields.G)
	{
		// 4096-bit granularity is enabled for this segment, scale the limit
		SegmentSelector->LIMIT = (SegmentSelector->LIMIT << 12) + 0xfff;
	}

	return TRUE;
}

VOID FillGuestSelectorData(PVOID  GdtBase, ULONG  Segreg, USHORT Selector) {
	VMCS_SEGMENT_SELECTOR SegmentSelector = { 0 };
	ULONG            AccessRights;

	GetSegmentDescriptor(&SegmentSelector, Selector, GdtBase);
	AccessRights = ((PUCHAR)&SegmentSelector.ATTRIBUTES)[0] + (((PUCHAR)&SegmentSelector.ATTRIBUTES)[1] << 12);

	if (!Selector)
		AccessRights |= 0x10000;

	__vmx_vmwrite(VMCS_GUEST_ES_SELECTOR + Segreg * 2, Selector);
	__vmx_vmwrite(VMCS_GUEST_ES_LIMIT + Segreg * 2, SegmentSelector.LIMIT);
	__vmx_vmwrite(VMCS_GUEST_ES_ACCESS_RIGHTS + Segreg * 2, AccessRights);
	__vmx_vmwrite(VMCS_GUEST_ES_BASE + Segreg * 2, SegmentSelector.BASE);
}

ULONG AdjustControls(ULONG Ctl, ULONG Msr) {
	LARGE_INTEGER MsrValue = { 0 };

	MsrValue.QuadPart = __readmsr(Msr);
	Ctl &= MsrValue.HighPart;	/* bit == 0 in high word ==> must be zero */
	Ctl |= MsrValue.LowPart;	/* bit == 1 in low word  ==> must be one  */

	return Ctl;
}

EVmErrors SetupVmcs() {

	//
	// Control Registers - Guest & Host
	//
	__vmx_vmwrite(VMCS_GUEST_CR0, __readcr0());
	__vmx_vmwrite(VMCS_GUEST_CR3, __readcr3());
	__vmx_vmwrite(VMCS_GUEST_CR4, __readcr4());

	__vmx_vmwrite(VMCS_HOST_CR0, __readcr0());
	__vmx_vmwrite(VMCS_HOST_CR3, __readcr3());
	__vmx_vmwrite(VMCS_HOST_CR4, __readcr4());

	DbgPrint("[*] Guest & Host Control Registers done.\n");

	// Don't even know why Daax included this in his hypervisor series.
	__vmx_vmwrite(VMCS_CTRL_CR0_READ_SHADOW, __readcr0());
	__vmx_vmwrite(VMCS_CTRL_CR4_READ_SHADOW, __readcr4());

	//
	// Debug Register (DR7)
	//
	__vmx_vmwrite(VMCS_GUEST_DR7, __readdr(7));
	DbgPrint("[*] Guest DR7 Done.\n");

	//
	// RSP, RIP, RFLAGS - Guest & Host
	//
	__vmx_vmwrite(VMCS_GUEST_RSP, g_GuestMemory);
	__vmx_vmwrite(VMCS_GUEST_RIP, g_GuestMemory);
	__vmx_vmwrite(VMCS_GUEST_RFLAGS, __readeflags());

	__vmx_vmwrite(VMCS_HOST_RSP, vmm_context->GuestStack + STACK_SIZE - 1);
	// Address host should point to, to kick things off when vmexit occurs
	__vmx_vmwrite(VMCS_HOST_RIP, (UINT64)HostContinueExecution);
	DbgPrint("[*] Guest & Host RSP, RIP, RFLAGS done\n");

	//
	// CS, SS, DS, ES, FS, GS, LDTR, and TR -- Guest & Host
	//
	CONTEXT ctx;
	RtlCaptureContext(&ctx);


	

	__vmx_vmwrite(VMCS_GUEST_CS_SELECTOR, vmxGdtEntry.Selector);
	__vmx_vmwrite(VMCS_GUEST_CS_BASE, vmxGdtEntry.Base);
	__vmx_vmwrite(VMCS_GUEST_CS_LIMIT, vmxGdtEntry.Limit);
	__vmx_vmwrite(VMCS_GUEST_CS_ACCESS_RIGHTS, vmxGdtEntry.AccessRights);

	__vmx_vmwrite(VMCS_HOST_CS_SELECTOR, (ctx.SegCs & 0xF8));
	DbgPrint("[*] Guest & Host CS done\n");

	//ShvUtilConvertGdtEntry((void*)gdtrBase, ctx.SegSs, &vmxGdtEntry);

	__vmx_vmwrite(VMCS_GUEST_SS_SELECTOR, vmxGdtEntry.Selector);
	__vmx_vmwrite(VMCS_GUEST_SS_BASE, vmxGdtEntry.Base);
	__vmx_vmwrite(VMCS_GUEST_SS_LIMIT, vmxGdtEntry.Limit);
	__vmx_vmwrite(VMCS_GUEST_SS_ACCESS_RIGHTS, vmxGdtEntry.AccessRights);

	__vmx_vmwrite(VMCS_HOST_SS_SELECTOR, (ctx.SegSs & 0xF8));
	DbgPrint("[*] Guest & Host SS done\n");

	//ShvUtilConvertGdtEntry((void*)gdtrBase, ctx.SegDs, &vmxGdtEntry);

	__vmx_vmwrite(VMCS_GUEST_DS_SELECTOR, vmxGdtEntry.Selector);
	__vmx_vmwrite(VMCS_GUEST_DS_BASE, vmxGdtEntry.Base);
	__vmx_vmwrite(VMCS_GUEST_DS_LIMIT, vmxGdtEntry.Limit);
	__vmx_vmwrite(VMCS_GUEST_DS_ACCESS_RIGHTS, vmxGdtEntry.AccessRights);

	__vmx_vmwrite(VMCS_HOST_DS_SELECTOR, (ctx.SegDs & 0xF8));
	DbgPrint("[*] Guest & Host DS done\n");

	//ShvUtilConvertGdtEntry((void*)gdtrBase, ctx.SegEs, &vmxGdtEntry);

	__vmx_vmwrite(VMCS_GUEST_ES_SELECTOR, vmxGdtEntry.Selector);	// GETES() & 0xF8
	__vmx_vmwrite(VMCS_GUEST_ES_BASE, vmxGdtEntry.Base);
	__vmx_vmwrite(VMCS_GUEST_ES_LIMIT, vmxGdtEntry.Limit);
	__vmx_vmwrite(VMCS_GUEST_ES_ACCESS_RIGHTS, vmxGdtEntry.AccessRights);

	__vmx_vmwrite(VMCS_HOST_ES_SELECTOR, (ctx.SegEs & 0xF8));
	DbgPrint("[*] Guest & Host ES done\n");

	//ShvUtilConvertGdtEntry((void*)gdtrBase, ctx.SegFs, &vmxGdtEntry);

	__vmx_vmwrite(VMCS_GUEST_FS_SELECTOR, vmxGdtEntry.Selector);
	__vmx_vmwrite(VMCS_GUEST_FS_BASE, __readmsr(IA32_FS_BASE));
	__vmx_vmwrite(VMCS_GUEST_FS_LIMIT, vmxGdtEntry.Limit);
	__vmx_vmwrite(VMCS_GUEST_FS_ACCESS_RIGHTS, vmxGdtEntry.AccessRights);

	__vmx_vmwrite(VMCS_HOST_FS_SELECTOR, (ctx.SegFs & 0xF8));
	__vmx_vmwrite(VMCS_HOST_FS_BASE, __readmsr(IA32_FS_BASE));
	DbgPrint("[*] Guest & Host FS done\n");

	//ShvUtilConvertGdtEntry((void*)gdtrBase, ctx.SegGs, &vmxGdtEntry);

	__vmx_vmwrite(VMCS_GUEST_GS_SELECTOR, vmxGdtEntry.Selector);
	__vmx_vmwrite(VMCS_GUEST_GS_BASE, __readmsr(IA32_GS_BASE));
	__vmx_vmwrite(VMCS_GUEST_GS_LIMIT, vmxGdtEntry.Limit);
	__vmx_vmwrite(VMCS_GUEST_GS_ACCESS_RIGHTS, vmxGdtEntry.AccessRights);

	__vmx_vmwrite(VMCS_HOST_GS_SELECTOR, (ctx.SegGs & 0xF8));
	__vmx_vmwrite(VMCS_HOST_GS_BASE, __readmsr(IA32_GS_BASE));
	DbgPrint("[*] Guest & Host GS done\n");


	ldtr = (UINT16)GetLdtr();
	//ShvUtilConvertGdtEntry((void*)gdtrBase, ldtr, &vmxGdtEntry);

	__vmx_vmwrite(VMCS_GUEST_LDTR_SELECTOR, vmxGdtEntry.Selector);
	__vmx_vmwrite(VMCS_GUEST_LDTR_BASE, vmxGdtEntry.Base);
	__vmx_vmwrite(VMCS_GUEST_LDTR_LIMIT, vmxGdtEntry.Limit);
	__vmx_vmwrite(VMCS_GUEST_LDTR_ACCESS_RIGHTS, vmxGdtEntry.AccessRights);
	DbgPrint("[*] Guest ldtr done\n");

	// There is no field in the host - state area for the LDTR selector.


	tr = (UINT16)GetTr();
	//ShvUtilConvertGdtEntry((void*)gdtrBase, tr, &vmxGdtEntry);

	__vmx_vmwrite(VMCS_GUEST_TR_SELECTOR, vmxGdtEntry.Selector);	// GETTR() & 0xF8
	__vmx_vmwrite(VMCS_GUEST_TR_BASE, vmxGdtEntry.Base);
	__vmx_vmwrite(VMCS_GUEST_TR_LIMIT, vmxGdtEntry.Limit);
	__vmx_vmwrite(VMCS_GUEST_TR_ACCESS_RIGHTS, vmxGdtEntry.AccessRights);

	__vmx_vmwrite(VMCS_HOST_TR_SELECTOR, (tr & 0xF8));
	__vmx_vmwrite(VMCS_HOST_TR_BASE, vmxGdtEntry.Base);
	DbgPrint("[*] Guest & Host Tr done\n");

	//
	// GDTR and IDTR 
	//

	unsigned char idtr[10] = { 0 };
	__sidt(idtr);
	unsigned long long idtrBase = (unsigned long long)idtr[9] << 56 |
		(unsigned long long)idtr[8] << 48 |
		(unsigned long long)idtr[7] << 40 |
		(unsigned long long)idtr[6] << 32 |
		(unsigned long long)idtr[5] << 24 |
		(unsigned long long)idtr[4] << 16 |
		(unsigned long long)idtr[3] << 8 |
		(unsigned long long)idtr[2];
	unsigned short idtrLimit = (unsigned int)idtr[1] << 8 |
		(unsigned int)idtr[0];

	__vmx_vmwrite(VMCS_GUEST_GDTR_BASE, gdtrBase);
	__vmx_vmwrite(VMCS_GUEST_GDTR_LIMIT, gdtrLimit);
	__vmx_vmwrite(VMCS_HOST_GDTR_BASE, gdtrBase);
	DbgPrint("[*] Guest & Host Gdtr done\n");

	__vmx_vmwrite(VMCS_GUEST_IDTR_BASE, idtrBase);
	__vmx_vmwrite(VMCS_GUEST_IDTR_LIMIT, idtrLimit);
	__vmx_vmwrite(VMCS_HOST_IDTR_BASE, idtrBase);
	DbgPrint("[*] Guest & Host Idtr done\n");

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
	DbgPrint("[*] Guest & Host MSR done\n");

	//
	// SMBASE (32 bits)
	// This register contains the base address of the logical processor s SMRAM image.
	// 
	//__vmx_vmwrite(VMCS_GUEST_SMBASE, );

	//
	// VMCS link pointer
	//
	__vmx_vmwrite(VMCS_GUEST_VMCS_LINK_POINTER, (size_t)(~0));
	DbgPrint("[*] VMCS link pointer done\n");

	//
	// VM Execution Control Fields
	// These fields control processor behavior in VMX non-root operation.
	// They determine in part the causes of VM exits.
	//
	__vmx_vmwrite(VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS, AdjustControls(0, IA32_VMX_PINBASED_CTLS));

	__vmx_vmwrite(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS,
		AdjustControls(IA32_VMX_PROCBASED_CTLS_HLT_EXITING_FLAG | IA32_VMX_PROCBASED_CTLS_ACTIVATE_SECONDARY_CONTROLS_FLAG,
			IA32_VMX_PROCBASED_CTLS));
	__vmx_vmwrite(VMCS_CTRL_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS,
		AdjustControls(/*IA32_VMX_PROCBASED_CTLS2_ENABLE_EPT_FLAG |*/ IA32_VMX_PROCBASED_CTLS2_ENABLE_RDTSCP_FLAG,
			IA32_VMX_PROCBASED_CTLS2));
	DbgPrint("[*] VM Execution fields done\n");

	//
	// VM-exit control fields. 
	// These fields control VM exits
	//
	__vmx_vmwrite(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS,
		AdjustControls(IA32_VMX_EXIT_CTLS_HOST_ADDRESS_SPACE_SIZE_FLAG | IA32_VMX_EXIT_CTLS_ACKNOWLEDGE_INTERRUPT_ON_EXIT_FLAG,
			IA32_VMX_EXIT_CTLS));
	DbgPrint("[*] VM Exit fields done\n");

	//
	// VM-entry control fields. 
	// These fields control VM entries.
	//
	__vmx_vmwrite(VMCS_CTRL_VMENTRY_CONTROLS,
		AdjustControls(IA32_VMX_ENTRY_CTLS_IA32E_MODE_GUEST_FLAG, IA32_VMX_ENTRY_CTLS));
	DbgPrint("[*] VM Entry control fields done\n");

	//
	// VM-exit information fields. 
	// These fields receive information on VM exits and describe the cause and the nature of VM exits.
	//

	// No information is stored here.
	//__vmx_vmwrite(VMCS_VMEXIT_INSTRUCTION_INFO,
	//	AdjustControls(, IA32_VMX_EXIT_CTLS));

	//
	// Misc
	//
	__vmx_vmwrite(VMCS_GUEST_ACTIVITY_STATE, 0);	// Active State
	__vmx_vmwrite(VMCS_GUEST_INTERRUPTIBILITY_STATE, 0);
	DbgPrint("[*] Misc done.\n");

	//
	// let's make some check here.
	// Don't know if they are really needed.
	//
	IA32_VMX_MISC_REGISTER misc;
	misc.AsUInt = __readmsr(IA32_VMX_MISC);
	DbgPrint("[*][Debugging] CR3 Target count : %llx\n", misc.Cr3TargetCount);	// If > 4, vmluanch fails.

	
	return VM_ERROR_OK;
}