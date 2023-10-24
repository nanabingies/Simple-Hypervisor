#pragma once
#define numPagesToAllocate	10
#define numMtrrEntries		255

enum MtrrMemoryType {
	Uncacheable = 00,
	WriteCombining,
	Reserved1,
	Reserved2,
	WriteThrough,
	WriteProtect,
	WriteBack,
};

struct MtrrEntry {
	UINT64	MtrrEnabled;
	UINT64	MemoryType;
	UINT64	PhysicalAddressStart;
	UINT64	PhysicalAddressEnd;
};

BOOLEAN CheckEPTSupport();

VOID InitializeEpt(UCHAR);

BOOLEAN EptBuildMTRRMap();