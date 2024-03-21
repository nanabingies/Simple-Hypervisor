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

using ia32_mtrr_fixed_range_msr = union ia32_mtrr_fixed_range_msr {
	uint64_t all;
	struct {
		uchar types[8];
	} fields;
} ;

enum mtrr_memory_type {
	Uncacheable = 0l,
	WriteCombining,
	Reserved1,
	Reserved2,
	WriteThrough,
	WriteProtect,
	WriteBack,
};

enum inv_ept_type {
	IndividualAddressInvalidation = 0,
	SingleContextInvalidation = 1,
	GlobalInvalidation = 2,
	SingleContextInvalidationExceptGlobal = 3,
};

using mtrr_entry = struct _mtrr_entry {
	bool		mtrr_enabled;
	bool		mtrr_fixed;
	uint64_t	memory_type;
	uint64_t	physical_address_start;
	uint64_t	physical_address_end;
};

using ept_split_page = struct _ept_split_page {
	DECLSPEC_ALIGN(PAGE_SIZE)	ept_pte ept_pte[512];
	ept_pde_2mb*				ept_pde;
	LIST_ENTRY					split_pages;
};

using ept_page_table = struct _ept_page_table {
	DECLSPEC_ALIGN(PAGE_SIZE)	ept_pml4e ept_pml4[EPTPML4ENTRIES];
	DECLSPEC_ALIGN(PAGE_SIZE)	ept_pdpte ept_pdpte[EPTPDPTEENTRIES];
	DECLSPEC_ALIGN(PAGE_SIZE)	ept_pde_2mb ept_pde[EPTPML4ENTRIES][EPTPDPTEENTRIES];
	ept_entry**					dynamic_pages;
	uint64_t					dynamic_pages_count;
};

using ept_state = struct _ept_state {
	uint64_t		guest_address_width_value;
	ept_pointer*	ept_ptr;
	ept_page_table* ept_page_table;
};

namespace ept {
	auto check_ept_support() -> bool;
}