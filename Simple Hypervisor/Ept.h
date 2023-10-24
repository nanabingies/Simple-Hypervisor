#pragma once
#define numPagesToAllocate	10
#define numMtrrEntries		255

struct MtrrEntry {
	UINT64 PhysicalAddressStart;
	UINT64 PhysicalAddressEnd;
};

BOOLEAN CheckEPTSupport();

VOID InitializeEpt();

BOOLEAN EptBuildMTRRMap();