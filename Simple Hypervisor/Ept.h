#pragma once
#define numPagesToAllocate	10
#define numMtrrEntries		255

typedef union Ia32MtrrFixedRangeMsr {
	UINT64 all;
	struct {
		UCHAR types[8];
	} fields;
} Ia32MtrrFixedRangeMsr;

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
	BOOLEAN	MtrrEnabled;
	BOOLEAN MtrrFixed;
	UINT64	MemoryType;
	UINT64	PhysicalAddressStart;
	UINT64	PhysicalAddressEnd;
};

struct EptState {
	UINT64	EptPtr;
	UINT64	GuestAddressWidthValue;
	UINT64	EptPml4[512];
	UINT64	EptPdpte[512 * 512];
};

static const ULONG MaxEptWalkLength = 0x4;

UINT64 g_DefaultMemoryType;

BOOLEAN CheckEPTSupport();

VOID InitializeEpt(UCHAR);

BOOLEAN EptBuildMTRRMap();