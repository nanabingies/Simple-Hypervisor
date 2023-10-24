#pragma once
#define numPagesToAllocate	10
#define numMtrrEntries		255

struct MtrrEntry {
	UINT64	MtrrEnabled;
	UINT64	MemoryType;
	UINT64	PhysicalAddressStart;
	UINT64	PhysicalAddressEnd;
};

BOOLEAN CheckEPTSupport();

VOID InitializeEpt();

BOOLEAN EptBuildMTRRMap();