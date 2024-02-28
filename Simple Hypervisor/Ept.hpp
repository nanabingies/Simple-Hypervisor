#pragma once
#include "stdafx.h"

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
	DECLSPEC_ALIGN(PAGE_SIZE)	ept_pte EptPte[512];
	ept_pde_2mb*				EptPde;
	LIST_ENTRY					SplitPages;
};

using EptPageTable = struct _EptPageTable {
	DECLSPEC_ALIGN(PAGE_SIZE)	ept_pml4e EptPml4[EPTPML4ENTRIES];
	DECLSPEC_ALIGN(PAGE_SIZE)	ept_pdpte EptPdpte[EPTPDPTEENTRIES];
	DECLSPEC_ALIGN(PAGE_SIZE)	ept_pde_2mb EptPde[EPTPML4ENTRIES][EPTPDPTEENTRIES];
	ept_entry** DynamicPages;
	uint64_t					DynamicPagesCount;
};

using EptState = struct _EptState{
	uint64_t		GuestAddressWidthValue;
	ept_pointer*	EptPtr;
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

auto SetupPml2Entries(EptState*, ept_pde_2mb*, UINT64) -> void;

auto IsInRange(uint64_t, uint64_t, uint64_t) -> bool;

auto IsValidForLargePage(uint64_t) -> bool;

auto GetPteEntry(EptPageTable*, uint64_t) -> ept_pte*;

auto GetPdeEntry(EptPageTable*, uint64_t) -> ept_pde_2mb*;

const auto EptConstructTables(ept_entry*, uint64_t, uint64_t, EptPageTable*) -> ept_entry*;

auto EptInitTableEntry(ept_entry*, uint64_t, uint64_t) -> void;

auto EptAllocateEptEntry(EptPageTable*) -> ept_entry*;

auto EptGetMemoryType(uint64_t, bool) -> uint64_t;

auto EptInvGlobalEntry() -> uint64_t;

auto SplitPde(EptPageTable*, void*, uint64_t) -> void;
