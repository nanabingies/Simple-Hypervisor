#pragma once
#include "stdafx.h"

#define num_pages_to_allocate	10
#define num_mtrr_entries		255

#define EPTPML4ENTRIES		512
#define EPTPDPTEENTRIES		512
#define EPTPDEENTRIES		512 * 512

#define	DYNAMICPAGESCOUNT	1
#define PAGE2MB				512 * PAGE_SIZE

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
	bool		mtrr_enabled;
	bool		mtrr_fixed;
	unsigned __int64	memory_type;
	unsigned __int64	physical_address_start;
	unsigned __int64	physical_address_end;
};

using ept_split_page = struct _ept_split_page {
	DECLSPEC_ALIGN(PAGE_SIZE)	ept_pte ept_pte[512];
	ept_pde_2mb* ept_pde;
	LIST_ENTRY					split_pages;
};

using ept_page_table = struct _ept_page_table {
	DECLSPEC_ALIGN(PAGE_SIZE)	ept_pml4e ept_pml4[EPTPML4ENTRIES];
	DECLSPEC_ALIGN(PAGE_SIZE)	ept_pdpte ept_pdpte[EPTPDPTEENTRIES];
	DECLSPEC_ALIGN(PAGE_SIZE)	ept_pde_2mb ept_pde[EPTPML4ENTRIES][EPTPDPTEENTRIES];
	ept_entry** dynamic_pages;
	unsigned __int64					dynamic_pages_count;
};

using ept_state = struct _ept_state {
	unsigned __int64		guest_address_width_value;
	ept_pointer* ept_ptr;
	ept_page_table* ept_page_table;
};

using vmx_non_root_memory = struct vmx_non_root_memory {
	void* pre_allocated_buffer;
};

namespace ept {
	static inline unsigned __int64 g_mtrr_num = 0;
	static inline unsigned __int64 max_ept_walk_length = 0x4;

	inline unsigned __int64 g_default_memory_type;

	auto check_ept_support() -> bool;

	auto initialize_ept(unsigned char) -> bool;

	auto ept_build_mtrr_map() -> bool;

	auto create_ept_state(ept_state*) -> bool;

	auto handle_ept_violation(unsigned __int64, unsigned __int64) -> void;

	auto setup_pml2_entries(ept_state*, ept_pde_2mb*, unsigned __int64) -> void;

	auto is_in_range(unsigned __int64, unsigned __int64, unsigned __int64) -> bool;

	auto is_valid_for_large_page(unsigned __int64) -> bool;

	auto get_pte_entry(ept_page_table*, unsigned __int64) -> ept_pte*;

	auto get_pde_entry(ept_page_table*, unsigned __int64) -> ept_pde_2mb*;

	auto ept_construct_tables(ept_entry*, unsigned __int64, unsigned __int64, ept_page_table*) -> ept_entry*;

	auto ept_init_table_entry(ept_entry*, unsigned __int64, unsigned __int64) -> void;

	auto ept_allocate_ept_entry(ept_page_table*) -> ept_entry*;

	auto ept_get_memory_type(unsigned __int64, bool) -> unsigned __int64;

	auto ept_inv_global_entry() -> unsigned __int64;

	auto split_pde(ept_page_table*, void*, unsigned __int64) -> void;
}
