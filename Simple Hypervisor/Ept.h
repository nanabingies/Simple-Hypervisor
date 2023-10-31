#pragma once
#include "ia32.h"

#define numPagesToAllocate	10
#define numMtrrEntries		255

#define EPTPML4ENTRIES		512
#define EPTPDPTEENTRIES		512
#define EPTPDEENTRIES		512 * 512

#define PAGE2MB				512 * PAGE_SIZE

#define MASK_EPT_PML1_INDEX(_VAR_) ((_VAR_ & 0x1FF000ULL) >> 12)
#define MASK_EPT_PML2_INDEX(_VAR_) ((_VAR_ & 0x3FE00000ULL) >> 21)
#define MASK_EPT_PML3_INDEX(_VAR_) ((_VAR_ & 0x7FC0000000ULL) >> 30)
#define MASK_EPT_PML4_INDEX(_VAR_) ((_VAR_ & 0xFF8000000000ULL) >> 39)

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

/*enum InvEptType {
	IndividualAddressInvalidation = 0,
	SingleContextInvalidation = 1,
	GlobalInvalidation = 2,
	SingleContextInvalidationExceptGlobal = 3,
};*/

typedef struct _MtrrEntry {
	BOOLEAN	MtrrEnabled;
	BOOLEAN MtrrFixed;
	UINT64	MemoryType;
	UINT64	PhysicalAddressStart;
	UINT64	PhysicalAddressEnd;
}MtrrEntry;

typedef struct _EptSplitPage {
	DECLSPEC_ALIGN(PAGE_SIZE)	EPT_PTE EptPte[512];
	EPT_PDE_2MB*				EptPde;
	LIST_ENTRY					SplitPages;
}EptSplitPage;

typedef struct _EptPageTable {
	DECLSPEC_ALIGN(PAGE_SIZE)	EPT_PML4E EptPml4[EPTPML4ENTRIES];
	DECLSPEC_ALIGN(PAGE_SIZE)	EPT_PDPTE EptPdpte[EPTPDPTEENTRIES];
	DECLSPEC_ALIGN(PAGE_SIZE)	EPT_PDE_2MB EptPde[EPTPML4ENTRIES][EPTPDPTEENTRIES];
	EPT_ENTRY**					DynamicPages;
	UINT64						EntriesCount;
} EptPageTable;

typedef struct _EptState {
	UINT64			GuestAddressWidthValue;
	EPT_POINTER*	EptPtr;
	EptPageTable*	EptPageTable;
} EptState;

typedef struct _VmxNonRootMemory {
	PVOID	PreAllocatedBuffer;
} VmxNonRootMemory;

static UINT64 gMtrrNum = 0;
static const ULONG MaxEptWalkLength = 0x4;

UINT64 g_DefaultMemoryType;

BOOLEAN CheckEPTSupport();

BOOLEAN InitializeEpt(UCHAR);

BOOLEAN EptBuildMTRRMap();

BOOLEAN CreateEptState(EptState*);

VOID HandleEptViolation(UINT64, UINT64);

VOID SetupPml2Entries(EptState*, EPT_PDE_2MB*, UINT64);

BOOLEAN IsInRange(UINT64, UINT64, UINT64);

BOOLEAN IsValidForLargePage(UINT64);

EPT_PTE* GetPteEntry(EptPageTable*, UINT64);

EPT_PDE_2MB* GetPdeEntry(EptPageTable*, UINT64);

EPT_ENTRY* EptpConstructTables(EPT_ENTRY*, UINT64, UINT64, EptPageTable*);

VOID EptInitTableEntry(EPT_ENTRY*, UINT64, UINT64);

EPT_ENTRY* EptAllocateEptEntry(EptPageTable*);

UINT64 EptGetMemoryType(UINT64, BOOLEAN);

UINT64 EptInvGlobalEntry();

VOID SplitPde(EptPageTable*, PVOID, UINT64);
