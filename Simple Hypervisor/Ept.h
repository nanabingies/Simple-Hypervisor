#pragma once
#define numPagesToAllocate	10
#define numMtrrEntries		255

enum MtrrMemoryType {
	Uncacheable = 0l,
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

struct EptState {
	UINT64 EptPtr;
};

const ULONG MaxEptWalkLength = 0x4;

UINT64 DefaultMemoryType;

BOOLEAN CheckEPTSupport();

VOID InitializeEpt(UCHAR);

BOOLEAN EptBuildMTRRMap();