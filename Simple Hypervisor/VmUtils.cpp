#include "stdafx.h"

auto PhysicalToVirtualAddress(uint64_t physical_address) -> uint64_t {
	PHYSICAL_ADDRESS physAddr;
	physAddr.QuadPart = physical_address;

	return reinterpret_cast<UINT64>(MmGetVirtualForPhysical(physAddr));
}

auto VirtualToPhysicalAddress(void* virtual_address) -> uint64_t {
	PHYSICAL_ADDRESS physAddr;

	physAddr = MmGetPhysicalAddress(virtual_address);
	return physAddr.QuadPart;
}
