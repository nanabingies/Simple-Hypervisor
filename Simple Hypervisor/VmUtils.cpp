#include "stdafx.h"

ULONG64 PhysicalToVirtualAddress(UINT64 physical_address) {
	PHYSICAL_ADDRESS physAddr;
	physAddr.QuadPart = physical_address;

	return (UINT64)(MmGetVirtualForPhysical(physAddr));
}

UINT64 VirtualToPhysicalAddress(void* virtual_address) {
	PHYSICAL_ADDRESS physAddr;

	physAddr = MmGetPhysicalAddress(virtual_address);
	return physAddr.QuadPart;
}
