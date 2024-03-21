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

namespace ept {
	auto check_ept_support() -> bool;
}