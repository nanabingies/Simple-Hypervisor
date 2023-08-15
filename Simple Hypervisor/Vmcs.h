#pragma once

#define RPL_MASK                3
#define SELECTOR_TABLE_INDEX    0x04

#define MSR_IA32_SYSENTER_CS	0x174
#define MSR_IA32_SYSENTER_ESP	0x175
#define MSR_IA32_SYSENTER_EIP	0x176
#define MSR_IA32_DEBUGCTL		0x1D9

#define VMCS_GUEST_DEBUGCTL_HIGH	0x2803

typedef enum EVmErrors
{
	VM_ERROR_OK = 0,
	VM_ERROR_ERR_INFO_OK,
	VM_ERROR_ERR_INFO_ERR,
}EVmErrors;

#define VM_OK(status) (status == EVmErrors::VM_ERROR_OK)
#define VMWRITE_ERR_QUIT(field, val) if (!VM_OK((status = VmWrite((field), (val))))) return status;

//void _sgdt(void* Destination);
//void __str(void* Destination);
//void __sldt(void* Destination);

UINT64 g_GuestRip;
UINT64 g_HostMemory;
UINT64 g_HostRip;

UINT64 g_StackPointerForReturning;
UINT64 g_BasePointerForReturning;

VOID
ShvUtilConvertGdtEntry(
	_In_ VOID* GdtBase,
	_In_ UINT16 Selector,
	_Out_ PVMX_GDTENTRY64 VmxGdtEntry
);

ULONG AdjustControls(ULONG Ctl, ULONG Msr);

EVmErrors SetupVmcs();