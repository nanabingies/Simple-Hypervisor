#pragma once
#include "ia32.hpp"

#define numPagesToAllocate	10
#define numMtrrEntries		255

#define EPTPML4ENTRIES		512
#define EPTPDPTEENTRIES		512
#define EPTPDEENTRIES		512 * 512

#define	DYNAMICPAGESCOUNT	1
#define PAGE2MB				512 * PAGE_SIZE

#define MASK_EPT_PML1_INDEX(_VAR_) ((_VAR_ & 0x1FF000ULL) >> 12)
#define MASK_EPT_PML2_INDEX(_VAR_) ((_VAR_ & 0x3FE00000ULL) >> 21)
#define MASK_EPT_PML3_INDEX(_VAR_) ((_VAR_ & 0x7FC0000000ULL) >> 30)
#define MASK_EPT_PML4_INDEX(_VAR_) ((_VAR_ & 0xFF8000000000ULL) >> 39)

typedef union Ia32MtrrFixedRangeMsr {
	uint64_t all;
	struct {
		uchar types[8];
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

using MtrrEntry = struct _MtrrEntry {
	bool MtrrEnabled;
	bool MtrrFixed;
	uint64_t MemoryType;
	uint64_t PhysicalAddressStart;
	uint64_t PhysicalAddressEnd;
};

using EptSplitPage = struct _EptSplitPage {
	DECLSPEC_ALIGN(PAGE_SIZE)	EPT_PTE EptPte[512];
	EPT_PDE_2MB*				EptPde;
	LIST_ENTRY					SplitPages;
};

using EptPageTable = struct _EptPageTable {
	DECLSPEC_ALIGN(PAGE_SIZE)	EPT_PML4E EptPml4[EPTPML4ENTRIES];
	DECLSPEC_ALIGN(PAGE_SIZE)	EPT_PDPTE EptPdpte[EPTPDPTEENTRIES];
	DECLSPEC_ALIGN(PAGE_SIZE)	EPT_PDE_2MB EptPde[EPTPML4ENTRIES][EPTPDPTEENTRIES];
	EPT_ENTRY** DynamicPages;
	UINT64						DynamicPagesCount;
};

using EptState = struct _EptState{
	uint64_t		GuestAddressWidthValue;
	EPT_POINTER*	EptPtr;
	EptPageTable*	EptPageTable;
};

using VmxNonRootMemory =  struct _VmxNonRootMemory {
	void*	PreAllocatedBuffer;
};

static uint64_t gMtrrNum = 0;
static const ulong MaxEptWalkLength = 0x4;

uint64_t g_DefaultMemoryType;

auto CheckEPTSupport() -> bool;

auto InitializeEpt(uchar) -> bool;

auto EptBuildMTRRMap() -> bool;

auto CreateEptState(EptState*) -> bool;

auto HandleEptViolation(uint64_t, uint64_t) -> void;

auto SetupPml2Entries(EptState*, EPT_PDE_2MB*, UINT64) -> void;

auto IsInRange(uint64_t, uint64_t, uint64_t) -> bool;

auto IsValidForLargePage(uint64_t) -> bool;

auto GetPteEntry(EptPageTable*, uint64_t) -> EPT_PTE*;

auto GetPdeEntry(EptPageTable*, uint64_t) -> EPT_PDE_2MB*;

const auto EptConstructTables(EPT_ENTRY*, uint64_t, uint64_t, EptPageTable*) -> EPT_ENTRY*;

auto EptInitTableEntry(EPT_ENTRY*, uint64_t, uint64_t) -> void;

auto EptAllocateEptEntry(EptPageTable*) -> EPT_ENTRY*;

auto EptGetMemoryType(uint64_t, bool) -> uint64_t;

auto EptInvGlobalEntry() -> uint64_t;

auto SplitPde(EptPageTable*, void*, uint64_t) -> void;
