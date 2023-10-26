#pragma once
#define numPagesToAllocate	10
#define numMtrrEntries		255

#define EPTPML4ENTRIES		512
#define EPTPDPTEENTRIE		512

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

typedef struct _MtrrEntry {
	BOOLEAN	MtrrEnabled;
	BOOLEAN MtrrFixed;
	UINT64	MemoryType;
	UINT64	PhysicalAddressStart;
	UINT64	PhysicalAddressEnd;
}MtrrEntry;

typedef struct _EptState {
	UINT64	EptPtr;
	UINT64	GuestAddressWidthValue;
	DECLSPEC_ALIGN(PAGE_SIZE)	UINT64	EptPml4[EPTPML4ENTRIES];
	DECLSPEC_ALIGN(PAGE_SIZE)	UINT64	EptPdpte[EPTPDPTEENTRIE];
	DECLSPEC_ALIGN(PAGE_SIZE)	UINT64	EptPde[EPTPML4ENTRIES][EPTPDPTEENTRIE];
}EptState;

static const ULONG MaxEptWalkLength = 0x4;

UINT64 g_DefaultMemoryType;

BOOLEAN CheckEPTSupport();

VOID InitializeEpt(UCHAR);

BOOLEAN EptBuildMTRRMap();