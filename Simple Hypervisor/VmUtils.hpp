#pragma once
#include "stdafx.h"
#pragma warning(disable : 4201)

#define DRV_NAME	L"\\Device\\Hypervisor"
#define DOS_NAME	L"\\DosDevices\\Hypervisor"
#define VMM_POOL	'tesT'

#define HOST_STACK_SIZE  (20 * PAGE_SIZE)
bool VmOff;

struct _vmm_context {
	uint64_t	vmxonRegionVirt;				// Virtual address of VMXON Region
	uint64_t	vmxonRegionPhys;				// Physical address of VMXON Region

	uint64_t	vmcsRegionVirt;					// virtual address of VMCS Region
	uint64_t	vmcsRegionPhys;					// Physical address of VMCS Region

	uint64_t  ioBitmapAVirt;                    // Virtual address of I/O bitmap A
	uint64_t  ioBitmapAPhys;                    // Physical address

	uint64_t	ioBitmapBVirt;					// Virtual address of I/O bitmap B
	uint64_t	ioBitmapBPhys;					// Plysical address

	uint64_t	msrBitmapVirt;					// Virtual address of MSR bitmap
	uint64_t	msrBitmapPhys;					// Physical address

	uint64_t	HostStack;						// Stack of the VM Exit Handler

	uint64_t	GuestMemory;					// Guest RSP
	
	uint64_t	GuestRsp;
	uint64_t	GuestRip;

	uint64_t	HostRsp;
	uint64_t	HostRip;

	struct {
		uint64_t	g_StackPointerForReturning;
		uint64_t	g_BasePointerForReturning;
	};

	uint64_t	EptPtr;
	uint64_t	EptPml4;
	EptState* EptState;
};

using ept_err = struct _eptErr {
	void*	Param1;
	void*	Param2;
} ;

struct driverGlobals {
	PDEVICE_OBJECT	g_DeviceObject;
	LIST_ENTRY		g_ListofDevices;
};

using GuestRegisters = struct _GuestRegisters {
	uint64_t	RAX;
	uint64_t	RBX;
	uint64_t	RCX;
	uint64_t	RDX;
	uint64_t	RSP;
	uint64_t	RBP;
	uint64_t	RSI;
	uint64_t	RDI;
	uint64_t	R8;
	uint64_t	R9;
	uint64_t	R10;
	uint64_t	R11;
	uint64_t	R12;
	uint64_t	R13;
	uint64_t	R14;
	uint64_t	R15;
	__m128	xmm[6];
} ;

using GuestContext = struct _GuestContext {
	GuestRegisters*	registers;
	uint64_t			ip;
} ;

_vmm_context* vmm_context;
driverGlobals g_DriverGlobals;
uint64_t g_GuestMemory;
uint64_t g_GuestRip;
uint64_t g_GuestRsp;

inline EVmErrors AsmInveptGlobal(uint64_t, const ept_err*);
inline EVmErrors AsmInveptContext();

uint64_t PhysicalToVirtualAddress(uint64_t physical_address);
uint64_t VirtualToPhysicalAddress(void* virtual_address);

extern "C" {
	NTKERNELAPI
		_IRQL_requires_max_(APC_LEVEL)
		_IRQL_requires_min_(PASSIVE_LEVEL) _IRQL_requires_same_ VOID
		KeGenericCallDpc(_In_ PKDEFERRED_ROUTINE Routine, _In_opt_ PVOID Context);

	extern "C"
		NTKERNELAPI
		_IRQL_requires_(DISPATCH_LEVEL) _IRQL_requires_same_ VOID
		KeSignalCallDpcDone(_In_ PVOID SystemArgument1);

	extern "C"
		NTKERNELAPI
		_IRQL_requires_(DISPATCH_LEVEL) _IRQL_requires_same_ LOGICAL
		KeSignalCallDpcSynchronize(_In_ PVOID SystemArgument2);
}
