#pragma once
#include "stdafx.h"

#define num_pages_to_allocate	10
#define num_mtrr_entries		255

#define EPTPML4ENTRIES		512
#define EPTPDPTEENTRIES		512
#define EPTPDEENTRIES		512 * 512

#define	DYNAMICPAGESCOUNT	1
#define PAGE2MB				512 * PAGE_SIZE

#define LARGE_PAGE_SIZE		0x200000

#define MASK_EPT_PML1_INDEX(_VAR_) ((_VAR_ & 0x1FF000ULL) >> 12)
#define MASK_EPT_PML2_INDEX(_VAR_) ((_VAR_ & 0x3FE00000ULL) >> 21)
#define MASK_EPT_PML3_INDEX(_VAR_) ((_VAR_ & 0x7FC0000000ULL) >> 30)
#define MASK_EPT_PML4_INDEX(_VAR_) ((_VAR_ & 0xFF8000000000ULL) >> 39)

using ia32_mtrr_fixed_range_msr = union _ia32_mtrr_fixed_range_msr {
	unsigned __int64 all;
	struct {
		unsigned char types[8];
	} fields;
};

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
	individual_address_invalidation = 0,
	single_context_invalidation = 1,
	global_invalidation = 2,
	single_context_invalidation_except_global = 3,
};



using mtrr_entry = struct _mtrr_entry {
	unsigned __int64	mtrr_enabled;
	unsigned __int64	mtrr_fixed;
	unsigned __int64	memory_type;
	unsigned __int64	physical_address_start;
	unsigned __int64	physical_address_end;
};

using mtrr_range_descriptor = struct _mtrr_range_descriptor {
	uint64_t	physical_base_address;
	uint64_t	physical_end_address;
	unsigned char mtrr_memory_type;
};

struct __ept_dynamic_split {
	DECLSPEC_ALIGN(PAGE_SIZE) ept_pte ept_pte[512];
	ept_pde_2mb* pde_entry;
	LIST_ENTRY dynamic_split_list;
};

using vmx_non_root_memory = struct vmx_non_root_memory {
	void* pre_allocated_buffer;
};

namespace ept {
	static inline unsigned __int64 g_mtrr_num = 0;
	static inline unsigned __int64 max_ept_walk_length = 0x4;

	inline unsigned __int64 g_default_memory_type;

	auto check_ept_support() -> bool;

	auto initialize_ept(__ept_state&) -> bool;

	auto ept_build_mtrr_map() -> bool;

	auto initialize_ept_page_table(__ept_state&) -> bool;

	auto setup_pml2_entries(__ept_state&, ept_pde_2mb&, unsigned __int64) -> bool;

	auto is_in_range(unsigned __int64, unsigned __int64, unsigned __int64) -> bool;

	auto is_valid_for_large_page(unsigned __int64) -> bool;

	auto split_pml2_entry(__ept_state&, void*, unsigned __int64) -> bool;

	//auto get_pte_entry(ept_page_table*, unsigned __int64) -> ept_pte*;

	auto get_pde_entry(__ept_state&, unsigned __int64) -> ept_pde_2mb*;

	//auto ept_construct_tables(ept_entry*, unsigned __int64, unsigned __int64, ept_page_table*) -> ept_entry*;

	auto ept_init_table_entry(ept_entry*, unsigned __int64, unsigned __int64) -> void;

	auto ept_allocate_ept_entry(__ept_page_table*) -> ept_entry*;

	auto ept_get_memory_type(unsigned __int64, bool) -> unsigned __int64;

	auto ept_inv_global_entry() -> unsigned __int64;

	//auto split_pde(ept_page_table*, void*, unsigned __int64) -> void;

	//auto handle_ept_violation(unsigned __int64, unsigned __int64) -> bool;
}
