#pragma once
#pragma warning(disable : 4201)

#define DRV_NAME	L"\\Device\\Hypervisor"
#define DOS_NAME	L"\\DosDevices\\Hypervisor"
#define VMM_POOL	'tesT'

#define HOST_STACK_SIZE  (20 * PAGE_SIZE)
BOOLEAN VmOff;

struct _vmm_context {
	UINT64	vmxonRegionVirt;				// Virtual address of VMXON Region
	UINT64	vmxonRegionPhys;				// Physical address of VMXON Region

	UINT64	vmcsRegionVirt;					// virtual address of VMCS Region
	UINT64	vmcsRegionPhys;					// Physical address of VMCS Region

	UINT64  ioBitmapAVirt;                    // Virtual address of I/O bitmap A
	UINT64  ioBitmapAPhys;                    // Physical address

	UINT64	ioBitmapBVirt;					// Virtual address of I/O bitmap B
	UINT64	ioBitmapBPhys;					// Plysical address

	UINT64	msrBitmapVirt;					// Virtual address of MSR bitmap
	UINT64	msrBitmapPhys;					// Physical address

	UINT64	HostStack;						// Stack of the VM Exit Handler

	UINT64	GuestMemory;					// Guest RSP
	
	UINT64	GuestRsp;
	UINT64	GuestRip;

	UINT64	HostRsp;
	UINT64	HostRip;

	struct {
		UINT64	g_StackPointerForReturning;
		UINT64	g_BasePointerForReturning;
	};

	UINT64	EptPtr;
	UINT64	EptPml4;
	EptState* EptState;
};


struct driverGlobals {
	PDEVICE_OBJECT	g_DeviceObject;
	LIST_ENTRY		g_ListofDevices;
};

struct _GuestRegisters {
	UINT64	RAX;
	UINT64	RBX;
	UINT64	RCX;
	UINT64	RDX;
	UINT64	RSP;
	UINT64	RBP;
	UINT64	RSI;
	UINT64	RDI;
	UINT64	R8;
	UINT64	R9;
	UINT64	R10;
	UINT64	R11;
	UINT64	R12;
	UINT64	R13;
	UINT64	R14;
	UINT64	R15;
};

struct _vmm_context* vmm_context;
struct driverGlobals g_DriverGlobals;
UINT64 g_GuestMemory;
UINT64 g_GuestRip;
UINT64 g_GuestRsp;

inline EVmErrors AsmInveptGlobal();
inline EVmErrors AsmInveptContext();

ULONG64 PhysicalToVirtualAddress(UINT64 physical_address);
UINT64 VirtualToPhysicalAddress(void* virtual_address);

NTKERNELAPI
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL) _IRQL_requires_same_ VOID
KeGenericCallDpc(_In_ PKDEFERRED_ROUTINE Routine, _In_opt_ PVOID Context);

NTKERNELAPI
_IRQL_requires_(DISPATCH_LEVEL) _IRQL_requires_same_ VOID
KeSignalCallDpcDone(_In_ PVOID SystemArgument1);

NTKERNELAPI
_IRQL_requires_(DISPATCH_LEVEL) _IRQL_requires_same_ LOGICAL
KeSignalCallDpcSynchronize(_In_ PVOID SystemArgument2);
