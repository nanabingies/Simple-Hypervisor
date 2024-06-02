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

namespace hv_vmcs {
	auto dump_vmcs() -> void {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "-----------------------------------VMCS CORE %u DUMP-----------------------------------\r\n", KeGetCurrentProcessorIndex());

		size_t parm = 0;
		// Natural Guest Register State Fields
		__vmx_vmread(VMCS_GUEST_CR0, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_CR0: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_CR3, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_CR3: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_CR4, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_CR4: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_ES_BASE, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_ES_BASE: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_CS_BASE, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_CS_BASE: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_SS_BASE, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_SS_BASE: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_DS_BASE, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_DS_BASE: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_FS_BASE, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_FS_BASE: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_GS_BASE, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_GS_BASE: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_LDTR_BASE, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_LDTR_BASE: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_TR_BASE, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_TR_BASE: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_GDTR_BASE, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_GDTR_BASE: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_IDTR_BASE, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_IDTR_BASE: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_DR7, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_DR7: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_RSP, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_RSP: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_RIP, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_RIP: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_RFLAGS, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_RFLAGS: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_SYSENTER_ESP, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_SYSENTER_ESP: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_SYSENTER_EIP, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_SYSENTER_EIP: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_S_CET, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_S_CET: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_SSP, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_SSP: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_INTERRUPT_SSP_TABLE_ADDR, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_INTERRUPT_SSP_TABLE_ADDR: 0x%llX\n", parm);

		// 64-bit Guest Register State Fields
		__vmx_vmread(VMCS_GUEST_VMCS_LINK_POINTER, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_VMCS_LINK_POINTER: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_DEBUGCTL, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_DEBUG_CONTROL: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_PAT, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_PAT: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_EFER, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_EFER: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_PERF_GLOBAL_CTRL, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_PERF_GLOBAL_CONTROL: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_PDPTE0, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_PDPTE0: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_PDPTE1, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_PDPTE1: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_PDPTE2, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_PDPTE2: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_PDPTE3, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_PDPTE3: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_BNDCFGS, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_BNDCFGS: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_RTIT_CTL, &parm);
		LOG("[!] GUEST_RTIT_CTL: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_PKRS, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_PKRS: 0x%llX\n", parm);

		// 32-Bit Guest Register State Fields
		__vmx_vmread(VMCS_GUEST_PKRS, &parm);
		LOG("[!] GUEST_ES_LIMIT: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_CS_LIMIT, &parm);
		LOG("[!] GUEST_CS_LIMIT: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_SS_LIMIT, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_SS_LIMIT: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_DS_LIMIT, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_DS_LIMIT: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_FS_LIMIT, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_FS_LIMIT: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_GS_LIMIT, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_GS_LIMIT: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_LDTR_LIMIT, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_LDTR_LIMIT: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_TR_LIMIT, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_TR_LIMIT: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_GDTR_LIMIT, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_GDTR_LIMIT: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_IDTR_LIMIT, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_IDTR_LIMIT: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_ES_ACCESS_RIGHTS, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_ES_ACCESS_RIGHTS: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_CS_ACCESS_RIGHTS, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_CS_ACCESS_RIGHTS: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_SS_ACCESS_RIGHTS, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_SS_ACCESS_RIGHTS: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_DS_ACCESS_RIGHTS, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_DS_ACCESS_RIGHTS: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_FS_ACCESS_RIGHTS, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_FS_ACCESS_RIGHTS: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_GS_ACCESS_RIGHTS, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_GS_ACCESS_RIGHTS: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_LDTR_ACCESS_RIGHTS, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_LDTR_ACCESS_RIGHTS: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_TR_ACCESS_RIGHTS, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_TR_ACCESS_RIGHTS: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_INTERRUPTIBILITY_STATE, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_INTERRUPTIBILITY_STATE: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_ACTIVITY_STATE, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_ACTIVITY_STATE: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_SMBASE, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_SMBASE: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_SYSENTER_CS, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_SYSENTER_CS: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_VMX_PREEMPTION_TIMER_VALUE, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_VMX_PREEMPTION_TIMER_VALUE: 0x%llX\n", parm);

		// 16-Bit Guest Register State Fields
		__vmx_vmread(VMCS_GUEST_ES_SELECTOR, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_ES_SELECTOR: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_CS_SELECTOR, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_CS_SELECTOR: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_SS_SELECTOR, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_SS_SELECTOR: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_DS_SELECTOR, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_DS_SELECTOR: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_FS_SELECTOR, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_FS_SELECTOR: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_GS_SELECTOR, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_GS_SELECTOR: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_LDTR_SELECTOR, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_LDTR_SELECTOR: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_TR_SELECTOR, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_TR_SELECTOR: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_INTERRUPT_STATUS, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_GUEST_INTERRUPT_STATUS: 0x%llX\n", parm);
		__vmx_vmread(VMCS_GUEST_PML_INDEX, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_PML_INDEX: 0x%llX\n", parm);

		// Natural Host Register State Fields
		__vmx_vmread(VMCS_HOST_CR0, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HOST_CR0: 0x%llX\n", parm);
		__vmx_vmread(VMCS_HOST_CR3, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HOST_CR3: 0x%llX\n", parm);
		__vmx_vmread(VMCS_HOST_CR4, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HOST_CR4: 0x%llX\n", parm);
		__vmx_vmread(VMCS_HOST_FS_BASE, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HOST_FS_BASE: 0x%llX\n", parm);
		__vmx_vmread(VMCS_HOST_GS_BASE, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HOST_GS_BASE: 0x%llX\n", parm);
		__vmx_vmread(VMCS_HOST_TR_BASE, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HOST_TR_BASE: 0x%llX\n", parm);
		__vmx_vmread(VMCS_HOST_GDTR_BASE, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HOST_GDTR_BASE: 0x%llX\n", parm);
		__vmx_vmread(VMCS_HOST_IDTR_BASE, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HOST_IDTR_BASE: 0x%llX\n", parm);
		__vmx_vmread(VMCS_HOST_SYSENTER_ESP, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HOST_SYSENTER_ESP: 0x%llX\n", parm);
		__vmx_vmread(VMCS_HOST_SYSENTER_EIP, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HOST_SYSENTER_EIP: 0x%llX\n", parm);
		__vmx_vmread(VMCS_HOST_RSP, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HOST_RSP: 0x%llX\n", parm);
		__vmx_vmread(VMCS_HOST_RSP, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HOST_RIP: 0x%llX\n", parm);
		__vmx_vmread(VMCS_HOST_S_CET, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HOST_S_CET: 0x%llX\n", parm);
		__vmx_vmread(VMCS_HOST_SSP, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HOST_SSP: 0x%llX\n", parm);
		__vmx_vmread(VMCS_HOST_INTERRUPT_SSP_TABLE_ADDR, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HOST_INTERRUPT_SSP_TABLE_ADDR: 0x%llX\n", parm);

		// 64-bit Host Register State Fields
		__vmx_vmread(VMCS_HOST_PAT, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HOST_PAT: 0x%llX\n", parm);
		__vmx_vmread(VMCS_HOST_EFER, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HOST_EFER: 0x%llX\n", parm);
		__vmx_vmread(VMCS_HOST_PERF_GLOBAL_CTRL, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HOST_PERF_GLOBAL_CTRL: 0x%llX\n", parm);
		__vmx_vmread(VMCS_HOST_PKRS, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HOST_PKRS: 0x%llX\n", parm);

		// 32-bit Host Register State Fields
		__vmx_vmread(VMCS_HOST_SYSENTER_CS, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HOST_SYSENTER_CS: 0x%llX\n", parm);

		// 16-bit Host Register State Fields
		__vmx_vmread(VMCS_HOST_ES_SELECTOR, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HOST_ES_SELECTOR: 0x%llX\n", parm);
		__vmx_vmread(VMCS_HOST_CS_SELECTOR, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HOST_CS_SELECTOR: 0x%llX\n", parm);
		__vmx_vmread(VMCS_HOST_SS_SELECTOR, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HOST_SS_SELECTOR: 0x%llX\n", parm);
		__vmx_vmread(VMCS_HOST_DS_SELECTOR, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HOST_DS_SELECTOR: 0x%llX\n", parm);
		__vmx_vmread(VMCS_HOST_FS_SELECTOR, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HOST_FS_SELECTOR: 0x%llX\n", parm);
		__vmx_vmread(VMCS_HOST_GS_SELECTOR, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HOST_GS_SELECTOR: 0x%llX\n", parm);
		__vmx_vmread(VMCS_HOST_TR_SELECTOR, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HOST_TR_SELECTOR: 0x%llX\n", parm);

		// Natural Control Register State Fields
		__vmx_vmread(VMCS_CTRL_CR0_GUEST_HOST_MASK, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_CR0_GUEST_HOST_MASK: 0x%llX\n", parm);
		__vmx_vmread(VMCS_CTRL_CR4_GUEST_HOST_MASK, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_CR4_GUEST_HOST_MASK: 0x%llX\n", parm);
		__vmx_vmread(VMCS_CTRL_CR0_READ_SHADOW, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_CR0_READ_SHADOW: 0x%llX\n", parm);
		__vmx_vmread(VMCS_CTRL_CR4_READ_SHADOW, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_CR4_READ_SHADOW: 0x%llX\n", parm);
		__vmx_vmread(VMCS_CTRL_CR3_TARGET_VALUE_0, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_CR3_TARGET_VALUE_0: 0x%llX\n", parm);
		__vmx_vmread(VMCS_CTRL_CR3_TARGET_VALUE_1, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_CR3_TARGET_VALUE_1: 0x%llX\n", parm);
		__vmx_vmread(VMCS_CTRL_CR3_TARGET_VALUE_2, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_CR3_TARGET_VALUE_2: 0x%llX\n", parm);
		__vmx_vmread(VMCS_CTRL_CR3_TARGET_VALUE_3, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_CR3_TARGET_VALUE_3: 0x%llX\n", parm);

		// 64-bit Control Register State Fields
		__vmx_vmread(VMCS_CTRL_IO_BITMAP_A_ADDRESS, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_BITMAP_IO_A_ADDRESS: 0x%llX\n", parm);
		__vmx_vmread(VMCS_CTRL_IO_BITMAP_B_ADDRESS, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_BITMAP_IO_B_ADDRESS: 0x%llX\n", parm);
		__vmx_vmread(VMCS_CTRL_MSR_BITMAP_ADDRESS, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_MSR_BITMAPS_ADDRESS: 0x%llX\n", parm);
		/*DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_VMEXIT_MSR_STORE_ADDRESS: 0x%llX", vmread(CONTROL_VMEXIT_MSR_STORE_ADDRESS));
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
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_ENCLV_EXITING_BITMAP: 0x%llX", vmread(CONTROL_ENCLV_EXITING_BITMAP));*/

		// 32-bit Control Register State Fields
		__vmx_vmread(VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_PIN_BASED_VM_EXECUTION_CONTROLS: 0x%llX\n", parm);
		__vmx_vmread(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_PRIMARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS: 0x%llX\n", parm);
		__vmx_vmread(VMCS_CTRL_EXCEPTION_BITMAP, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_EXCEPTION_BITMAP: 0x%llX\n", parm);
		__vmx_vmread(VMCS_CTRL_PAGEFAULT_ERROR_CODE_MASK, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_PAGE_FAULT_ERROR_CODE_MASK: 0x%llX\n", parm);
		__vmx_vmread(VMCS_CTRL_PAGEFAULT_ERROR_CODE_MATCH, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_PAGE_FAULT_ERROR_CODE_MATCH: 0x%llX\n", parm);
		__vmx_vmread(VMCS_CTRL_CR3_TARGET_COUNT, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_CR3_TARGET_COUNT: 0x%llX\n", parm);
		__vmx_vmread(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_VM_EXIT_CONTROLS: 0x%llX\n", parm);
		__vmx_vmread(VMCS_CTRL_VMEXIT_MSR_STORE_ADDRESS, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_VM_EXIT_MSR_STORE_COUNT: 0x%llX\n", parm);
		__vmx_vmread(VMCS_CTRL_VMEXIT_MSR_LOAD_COUNT, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_VM_EXIT_MSR_LOAD_COUNT: 0x%llX\n", parm);
		__vmx_vmread(VMCS_CTRL_VMENTRY_CONTROLS, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_VM_ENTRY_CONTROLS: 0x%llX\n", parm);
		__vmx_vmread(VMCS_CTRL_VMENTRY_MSR_LOAD_COUNT, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_VM_ENTRY_MSR_LOAD_COUNT: 0x%llX\n", parm);
		__vmx_vmread(VMCS_CTRL_VMENTRY_INTERRUPTION_INFORMATION_FIELD, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_VM_ENTRY_INTERRUPTION_INFORMATION_FIELD: 0x%llX\n", parm);
		__vmx_vmread(VMCS_CTRL_VMENTRY_EXCEPTION_ERROR_CODE, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_VM_ENTRY_EXCEPTION_ERROR_CODE: 0x%llX\n", parm);
		__vmx_vmread(VMCS_CTRL_VMENTRY_INSTRUCTION_LENGTH, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_VM_ENTRY_INSTRUCTION_LENGTH: 0x%llX\n", parm);
		//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_TPR_THRESHOLD: 0x%llX", vmread(CONTROL_TPR_THRESHOLD));
		__vmx_vmread(VMCS_CTRL_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS: 0x%llX\n", parm);
		//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_PLE_GAP: 0x%llX", vmread(CONTROL_PLE_GAP));
		//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_PLE_WINDOW: 0x%llX", vmread(CONTROL_PLE_WINDOW));

		// 16-bit Control Register State Fields
		__vmx_vmread(VMCS_CTRL_VIRTUAL_PROCESSOR_IDENTIFIER, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_VIRTUAL_PROCESSOR_IDENTIFIER: 0x%llX\n", parm);
		__vmx_vmread(VMCS_CTRL_POSTED_INTERRUPT_NOTIFICATION_VECTOR, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_POSTED_INTERRUPT_NOTIFICATION_VECTOR: 0x%llX\n", parm);
		__vmx_vmread(VMCS_CTRL_EPTP_INDEX, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CONTROL_EPTP_INDEX: 0x%llX\n", parm);

		// Natural Read only Register State Fields
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "EXIT_QUALIFICATION: 0x%llX\n", parm);
		__vmx_vmread(VMCS_IO_RCX, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IO_RCX: 0x%llX\n", parm);
		__vmx_vmread(VMCS_IO_RSI, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IO_RSI: 0x%llX\n", parm);
		__vmx_vmread(VMCS_IO_RDI, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IO_RDI: 0x%llX\n", parm);
		__vmx_vmread(VMCS_IO_RIP, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "IO_RIP: 0x%llX\n", parm);
		__vmx_vmread(VMCS_EXIT_GUEST_LINEAR_ADDRESS, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_LINEAR_ADDRESS: 0x%llX\n", parm);

		// 64-bit Read only Register State Fields
		__vmx_vmread(VMCS_GUEST_PHYSICAL_ADDRESS, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GUEST_PHYSICAL_ADDRESS: 0x%llX\n", parm);

		// 32-bit Read only Register State Fields
		__vmx_vmread(VMCS_VM_INSTRUCTION_ERROR, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[!] VM_INSTRUCTION_ERROR: 0x%llX\n", parm);
		__vmx_vmread(VMCS_EXIT_REASON, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[!] EXIT_REASON: 0x%llX\n", parm);
		__vmx_vmread(VMCS_VMEXIT_INTERRUPTION_INFORMATION, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[!] VM_EXIT_INTERRUPTION_INFORMATION: 0x%llX\n", parm);
		__vmx_vmread(VMCS_VMEXIT_INTERRUPTION_ERROR_CODE, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[!] VM_EXIT_INTERRUPTION_ERROR_CODE: 0x%llX\n", parm);
		__vmx_vmread(VMCS_IDT_VECTORING_ERROR_CODE, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[!] IDT_VECTORING_INFORMATION_FIELD: 0x%llX\n", parm);
		__vmx_vmread(VMCS_IDT_VECTORING_ERROR_CODE, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[!] IDT_VECTORING_ERROR_CODE: 0x%llX\n", parm);
		__vmx_vmread(VMCS_VMEXIT_INSTRUCTION_LENGTH, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[!] VM_EXIT_INSTRUCTION_LENGTH: 0x%llX\n", parm);
		__vmx_vmread(VMCS_VMEXIT_INSTRUCTION_INFO, &parm);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[!] VM_EXIT_INSTRUCTION_INFORMATION: 0x%llX\n", parm);

		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "-----------------------------------VMCS CORE %u DUMP-----------------------------------\r\n", KeGetCurrentProcessorIndex());
	}
}

auto hv_setup_vmcs(struct __vcpu* vcpu, void* guest_rsp) -> void {
	/// Set VMCS state to inactive
	__vmx_vmclear(static_cast<unsigned long long*>(&vcpu->vmcs_physical));

	/// Make VMCS the current and active on that processor
	__vmx_vmptrld(static_cast<unsigned long long*>(&vcpu->vmcs_physical));

	memset(vcpu->vcpu_bitmaps.io_bitmap_a, 0xff, PAGE_SIZE);
	memset(vcpu->vcpu_bitmaps.io_bitmap_b, 0xff, PAGE_SIZE);
	memset(vcpu->vcpu_bitmaps.msr_bitmap, 0xff, PAGE_SIZE);

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

	if (__vmx_vmwrite(VMCS_HOST_RSP, reinterpret_cast<size_t>(vcpu->vmm_stack) + VMM_STACK_SIZE))	return;
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
	__vmx_vmwrite(VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS, 
		adjust_controls(0, IA32_VMX_PINBASED_CTLS));

	__vmx_vmwrite(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS,
		adjust_controls(IA32_VMX_PROCBASED_CTLS_HLT_EXITING_FLAG | IA32_VMX_PROCBASED_CTLS_USE_MSR_BITMAPS_FLAG |
			IA32_VMX_PROCBASED_CTLS_CR3_LOAD_EXITING_FLAG | IA32_VMX_PROCBASED_CTLS_CR3_STORE_EXITING_FLAG |
			IA32_VMX_PROCBASED_CTLS_ACTIVATE_SECONDARY_CONTROLS_FLAG | IA32_VMX_PROCBASED_CTLS_USE_IO_BITMAPS_FLAG,
			IA32_VMX_PROCBASED_CTLS));

	__vmx_vmwrite(VMCS_CTRL_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS,
		adjust_controls(IA32_VMX_PROCBASED_CTLS2_ENABLE_XSAVES_FLAG | IA32_VMX_PROCBASED_CTLS2_ENABLE_RDTSCP_FLAG | 
			IA32_VMX_PROCBASED_CTLS2_ENABLE_EPT_FLAG,
			IA32_VMX_PROCBASED_CTLS2));

	/// VM-exit control fields. 
	/// These fields control VM exits
	__vmx_vmwrite(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS,
		adjust_controls(IA32_VMX_EXIT_CTLS_HOST_ADDRESS_SPACE_SIZE_FLAG | IA32_VMX_EXIT_CTLS_ACKNOWLEDGE_INTERRUPT_ON_EXIT_FLAG,
			IA32_VMX_EXIT_CTLS));

	/// VM-entry control fields. 
	/// These fields control VM entries.
	__vmx_vmwrite(VMCS_CTRL_VMENTRY_CONTROLS,
		adjust_controls(IA32_VMX_ENTRY_CTLS_IA32E_MODE_GUEST_FLAG, IA32_VMX_ENTRY_CTLS));

	/// Misc
	if (__vmx_vmwrite(VMCS_GUEST_ACTIVITY_STATE, 0))	return;	// Active State
	if (__vmx_vmwrite(VMCS_GUEST_INTERRUPTIBILITY_STATE, 0))	return;
	if (__vmx_vmwrite(VMCS_GUEST_PENDING_DEBUG_EXCEPTIONS, 0))	return;

	__debugbreak();
	LOG("[*] ept pointer : %p\n", (void*)vcpu->ept_state->ept_pointer);
	LOG("[*] ept flags : %llx\n", vcpu->ept_state->ept_pointer->flags);

	if (__vmx_vmwrite(VMCS_CTRL_EPT_POINTER, vcpu->ept_state->ept_pointer->flags))	return;

	if (__vmx_vmwrite(VMCS_CTRL_IO_BITMAP_A_ADDRESS, vcpu->vcpu_bitmaps.io_bitmap_a_physical))	return;
	if (__vmx_vmwrite(VMCS_CTRL_IO_BITMAP_B_ADDRESS, vcpu->vcpu_bitmaps.io_bitmap_b_physical))	return;

	if (__vmx_vmwrite(VMCS_CTRL_MSR_BITMAP_ADDRESS, vcpu->vcpu_bitmaps.msr_bitmap_physical))	return;

	if (__vmx_vmwrite(VMCS_CTRL_VIRTUAL_PROCESSOR_IDENTIFIER, KeGetCurrentProcessorNumberEx(NULL) + 1))	return;

	ia32_vmx_misc_register misc;
	misc.flags = __readmsr(IA32_VMX_MISC);

	__vmx_vmwrite(VMCS_CTRL_CR3_TARGET_COUNT, misc.cr3_target_count);

	for (unsigned iter = 0; iter < misc.cr3_target_count; iter++) {
		__vmx_vmwrite(VMCS_CTRL_CR3_TARGET_VALUE_0 + (iter * 2), 0);
	}

}
