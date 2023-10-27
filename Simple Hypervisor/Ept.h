#pragma once
#include "ia32.h"

#define numPagesToAllocate	10
#define numMtrrEntries		255

#define EPTPML4ENTRIES		512
#define EPTPDPTEENTRIES		512
#define EPTPDEENTRIES		512 * 512

#define PAGE2MB				512 * PAGE_SIZE

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

typedef struct _EptPageTable {
	DECLSPEC_ALIGN(PAGE_SIZE)	EPT_PML4E EptPml4[EPTPML4ENTRIES];
	DECLSPEC_ALIGN(PAGE_SIZE)	EPT_PDPTE EptPdpte[EPTPDPTEENTRIES];
	DECLSPEC_ALIGN(PAGE_SIZE)	EPT_PDE_2MB EptPde[EPTPML4ENTRIES][EPTPDPTEENTRIES];
} EptPageTable;

typedef struct _EptState {
	UINT64			GuestAddressWidthValue;
	EPT_POINTER*	EptPtr;
	EptPageTable*	EptPageTable;
} EptState;

static const ULONG MaxEptWalkLength = 0x4;

UINT64 g_DefaultMemoryType;

BOOLEAN CheckEPTSupport();

BOOLEAN InitializeEpt(UCHAR);

BOOLEAN EptBuildMTRRMap();

BOOLEAN CreateEptState(EptState*);

VOID SetupPml2Entries(EptState*, EPT_PDE_2MB, UINT64);

BOOLEAN IsInRange(UINT64, UINT64, UINT64);

BOOLEAN IsValidForLargePage(UINT64);

UINT64 GetMemoryType(UINT64, BOOLEAN)
