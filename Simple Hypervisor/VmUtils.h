#pragma once

#define DRV_NAME	L"\\Device\\Hypervisor"
#define DOS_NAME	L"\\DosDevices\\Hypervisor"
#define VMM_POOL	'tesT'

#define STACK_SIZE  (8 * PAGE_SIZE)

struct _vmm_context {
	UINT64	vmxonRegionVirt;				// Virtual address of VMXON Region
	UINT64	vmxonRegionPhys;				// Physical address of VMXON Region

	UINT64	vmcsRegionVirt;					// virtual address of VMCS Region
	UINT64	vmcsRegionPhys;					// Physical address of VMCS Region

	UINT64    bitmapVirt;                     // Virtual address of bitmap bits
	UINT64    bitmapPhys;                     // Physical address

	UINT64	eptPtr;							// Pointer to the EPT

	UINT64	GuestStack;						// Stack of the VM Exit Handler
};


struct driverGlobals {
	PDEVICE_OBJECT	g_DeviceObject;
	LIST_ENTRY		g_ListofDevices;
};

struct _vmm_context* vmm_context;
struct driverGlobals* g_DriverGlobals;