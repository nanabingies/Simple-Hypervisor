#pragma once
#pragma warning(disable : 4201)

#define DRV_NAME	L"\\Device\\Hypervisor"
#define DOS_NAME	L"\\DosDevices\\Hypervisor"
#define VMM_POOL	'tesT'

#define STACK_SIZE  (8 * PAGE_SIZE)
BOOLEAN VmOff;

struct _vmm_context {
	UINT64	vmxonRegionVirt;				// Virtual address of VMXON Region
	UINT64	vmxonRegionPhys;				// Physical address of VMXON Region

	UINT64	vmcsRegionVirt;					// virtual address of VMCS Region
	UINT64	vmcsRegionPhys;					// Physical address of VMCS Region

	UINT64  bitmapAVirt;                    // Virtual address of bitmap A
	UINT64  bitmapAPhys;                    // Physical address

	UINT64	bitmapBVirt;					// Virtual address of bitmap B
	UINT64	bitmapBPhys;					// Plysical address

	UINT64	eptPtr;							// Pointer to the EPT

	UINT64	HostStack;						// Stack of the VM Exit Handler
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

union _VmexitInfo {
	UINT32	AsUint;
	struct {
		UINT32	BasicExitReason : 16;
		UINT32	Reserved1 : 1;
		UINT32	NotDefined1 : 10;
		UINT32	EnclaveMode : 1;
		UINT32	PendingMTF : 1;
		UINT32	VMXRootVmExit : 1;
		UINT32	NotDefined2 : 1;
		UINT32	VmEntryFailure : 1;
	};
};

struct _vmm_context* vmm_context;
struct driverGlobals g_DriverGlobals;
UINT64 g_GuestMemory;
UINT64 g_GuestRip;
UINT64 g_GuestRsp;

ULONG64 PhysicalToVirtualAddress(UINT64 physical_address);
UINT64 VirtualToPhysicalAddress(void* virtual_address);
