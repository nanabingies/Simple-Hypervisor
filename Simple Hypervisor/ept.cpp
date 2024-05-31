#include "ept.hpp"
#pragma warning(disable: 4996)

namespace ept {
	static struct _mtrr_entry g_mtrr_entries[num_mtrr_entries];

	auto check_ept_support() -> bool {
		PAGED_CODE();

		PROCESSOR_NUMBER processor_number;
		GROUP_AFFINITY affinity, old_affinity;

		unsigned num_processors = KeQueryActiveProcessorCount(NULL);

		for (unsigned iter = 0; iter < num_processors; iter++) {
			KeGetProcessorNumberFromIndex(iter, &processor_number);

			RtlSecureZeroMemory(&affinity, sizeof(GROUP_AFFINITY));
			affinity.Group = processor_number.Group;
			affinity.Mask = static_cast<KAFFINITY>(1) << processor_number.Number;
			KeSetSystemGroupAffinityThread(&affinity, &old_affinity);

			auto irql = KeRaiseIrqlToDpcLevel();

			ia32_vmx_ept_vpid_cap_register ept_cap;
			ept_cap.flags = __readmsr(IA32_VMX_EPT_VPID_CAP);

			if (!ept_cap.page_walk_length_4 || !ept_cap.memory_type_write_back || !ept_cap.invept ||
				!ept_cap.invept_single_context || !ept_cap.invept_all_contexts || !ept_cap.invvpid ||
				!ept_cap.invvpid_individual_address || !ept_cap.invvpid_all_contexts ||
				!ept_cap.invvpid_single_context || !ept_cap.invvpid_single_context_retain_globals) {
				return false;
			}

			KeLowerIrql(irql);
			KeRevertToUserGroupAffinityThread(&old_affinity);
		}

		return true;
	}

	auto is_in_range(unsigned __int64 val, unsigned __int64 start, unsigned __int64 end) -> bool {
		if (val > start && val <= end)
			return true;

		return false;
	}

	auto ept_build_mtrr_map() -> bool {
		ia32_mtrr_capabilities_register mtrr_cap{};
		ia32_mtrr_def_type_register mtrr_def{};
		ia32_mtrr_physbase_register mtrr_phys_base{};
		ia32_mtrr_physmask_register mtrr_phys_mask{};
		__mtrr_range_descriptor* descriptor{};

		mtrr_cap.flags = __readmsr(IA32_MTRR_CAPABILITIES);
		mtrr_def.flags = __readmsr(IA32_MTRR_DEF_TYPE);

		uint64_t var_cnt = mtrr_cap.variable_range_count;
		uint64_t fix_range_support = mtrr_cap.fixed_range_supported;
		uint64_t fix_range_enable = mtrr_def.fixed_range_mtrr_enable;
		g_default_memory_type = mtrr_def.default_memory_type;

		if (!fix_range_enable && !fix_range_support)	return false;

		g_vmm_context->mtrr_info.default_memory_type = Uncacheable;

		// Handle Fix Ranged MTRR

		static const uint64_t k64k_base = IA32_MTRR_FIX64K_BASE;
		static const uint64_t k64k_size = IA32_MTRR_FIX64K_SIZE;	// 64K
		static const uint64_t k16k_base = IA32_MTRR_FIX16K_BASE;
		static const uint64_t k16k_size = IA32_MTRR_FIX16K_SIZE;
		static const uint64_t k4k_base = IA32_MTRR_FIX4K_BASE;
		static const uint64_t k4k_size = IA32_MTRR_FIX4K_SIZE;

		// Let's first read Fix64K MTRR
		ia32_mtrr_fixed_range_msr msr64k;
		msr64k.all = __readmsr(IA32_MTRR_FIX64K_00000);
		for (unsigned idx = 0; idx < 8; idx++) {

			// Save the MTRR entries
			descriptor = &g_vmm_context->mtrr_info.memory_range[g_vmm_context->mtrr_info.enabled_memory_ranges++];
			descriptor->memory_type = msr64k.fields.types[idx];
			descriptor->physcial_base_address = k64k_base + (k64k_size * idx);
			descriptor->physcial_end_address = k64k_base + (k64k_size * idx) + (k64k_size - 1);
			descriptor->fixed_range = true;
		}


		// let's read Fix16k MTRR
		ia32_mtrr_fixed_range_msr msr16k;
		// start -- IA32_MTRR_FIX16K_80000	
		// end   -- IA32_MTRR_FIX16K_A0000

		for (unsigned start = IA32_MTRR_FIX16K_80000; start <= IA32_MTRR_FIX16K_A0000; start++) {
			msr16k.all = __readmsr(start);
			for (unsigned idx = 0; idx < 8; idx++) {

				// Save the MTRR
				descriptor = &g_vmm_context->mtrr_info.memory_range[g_vmm_context->mtrr_info.enabled_memory_ranges++];
				descriptor->memory_type = msr16k.fields.types[idx];
				descriptor->physcial_base_address = (k16k_base + (start * k16k_size * 8)) + (k16k_size * idx);
				descriptor->physcial_end_address = (k16k_base + (start * k16k_size * 8)) + (k16k_size * idx) + (k16k_size - 1);
				descriptor->fixed_range = true;
			}
		}


		// let's read Fix4k MTRR
		ia32_mtrr_fixed_range_msr msr4k;
		// start -- IA32_MTRR_FIX4K_C0000	
		// end   -- IA32_MTRR_FIX4K_F8000

		for (unsigned start = IA32_MTRR_FIX4K_C0000; start <= IA32_MTRR_FIX4K_F8000; start++) {
			msr4k.all = __readmsr(start);
			for (unsigned idx = 0; idx < 8; idx++) {

				// Save the MTRR
				descriptor = &g_vmm_context->mtrr_info.memory_range[g_vmm_context->mtrr_info.enabled_memory_ranges++];
				descriptor->memory_type = msr4k.fields.types[idx];
				descriptor->physcial_base_address = (k4k_base + (start * k4k_size * 8)) + (k4k_size * idx);
				descriptor->physcial_end_address = (k4k_base + (start * k4k_size * 8)) + (k4k_size * idx) + (k4k_size - 1);
				descriptor->fixed_range = true;
			}
		}

		for (unsigned iter = 0; iter < var_cnt; iter++) {
			mtrr_phys_base.flags = __readmsr(IA32_MTRR_PHYSBASE0 + (iter * 2));
			mtrr_phys_mask.flags = __readmsr(IA32_MTRR_PHYSMASK0 + (iter * 2));

			// The range is invalid
			if (!mtrr_phys_mask.valid)
				continue;

			descriptor = &g_vmm_context->mtrr_info.memory_range[g_vmm_context->mtrr_info.enabled_memory_ranges++];

			//
			// Calculate base address, physbase is truncated by 12 bits so we have to left shift it by 12
			//
			descriptor->physcial_base_address = mtrr_phys_base.page_frame_number << PAGE_SHIFT;

			//
			// Index of first bit set to one determines how much do we have to bit shift to get size of range
			// physmask is truncated by 12 bits so we have to left shift it by 12
			//
			unsigned long bits_in_mask = 0;
			_BitScanForward64(&bits_in_mask, mtrr_phys_mask.page_frame_number << PAGE_SHIFT);

			//
			// Calculate the end of range specified by mtrr
			//
			descriptor->physcial_end_address = descriptor->physcial_base_address + ((1ULL << bits_in_mask) - 1ULL);

			//
			// Get memory type of range
			//
			descriptor->memory_type = (unsigned __int8)mtrr_phys_base.type;
			descriptor->fixed_range = false;
		}

		return true;
	}

	auto initialize_ept(__ept_state& ept_state) -> bool {
		ept_state.ept_pointer = reinterpret_cast<ept_pointer*>(ExAllocatePoolWithTag(NonPagedPool, sizeof(ept_pointer), VMM_POOL_TAG));
		if (ept_state.ept_pointer == nullptr) {
			LOG("[!] Failed to allocate memory for pointer to ept.\n");
			LOG_ERROR(__FILE__, __LINE__);
			return false;
		}
		RtlSecureZeroMemory(ept_state.ept_pointer, sizeof(ept_pointer));

		if (initialize_ept_page_table(ept_state) == false) {
			DbgPrint("[!] Failed to setup ept page table entries.\n");
			LOG_ERROR(__FILE__, __LINE__);
			return false;
		}

		ept_state.ept_pointer->memory_type = g_vmm_context->mtrr_info.default_memory_type;
		ept_state.ept_pointer->page_walk_length = 3;
		ept_state.ept_pointer->page_frame_number = virtual_to_physical_address(&ept_state.ept_page_table->ept_pml4) >> PAGE_SHIFT;

		LOG("[*] EPT initialized on processor (%x)\n", KeGetCurrentProcessorNumber());
		return true;
	}

	auto initialize_ept_page_table(__ept_state& ept_state) -> bool {
		ept_state.ept_page_table = reinterpret_cast<__ept_page_table*>
			(ExAllocatePoolWithTag(NonPagedPool, sizeof(__ept_page_table), VMM_POOL_TAG));
		if (page_table == nullptr)	return false;
		RtlSecureZeroMemory(ept_state.ept_page_table, sizeof(__ept_page_table));

		ept_state.ept_page_table->ept_pml4->page_frame_number = (virtual_to_physical_address(&ept_state.ept_page_table->ept_pdpte[0]) >> PAGE_SHIFT);
		ept_state.ept_page_table->ept_pml4->read_access = 1;
		ept_state.ept_page_table->ept_pml4->execute_access = 1;
		ept_state.ept_page_table->ept_pml4->write_access = 1;
		ept_state.ept_page_table->ept_pml4->accessed = 1;

		ept_pdpte pdpte_template = { 0 };
		pdpte_template.read_access = 1;
		pdpte_template.write_access = 1;
		pdpte_template.execute_access = 1;

		__stosq((unsigned __int64*)&ept_state.ept_page_table->ept_pdpte[0], pdpte_template.flags, 512);
		for (unsigned idx = 0; idx < 512; idx++) {
			ept_state.ept_page_table->ept_pdpte[idx].page_frame_number = 
				(virtual_to_physical_address(&ept_state.ept_page_table->ept_pde[idx][0]) >> PAGE_SHIFT);
		}

		ept_pde_2mb pde_template = { 0 };
		pde_template.read_access = 1;
		pde_template.write_access = 1;
		pde_template.execute_access = 1;
		pde_template.large_page = 1;

		__stosq((unsigned __int64*)&ept_state.ept_page_table->ept_pde[0], pde_template.flags, 512 * 512);
		for (unsigned i = 0; i < 512; i++) {
			for (unsigned j = 0; j < 512; j++) {
				if (setup_pml2_entries(ept_state, ept_state.ept_page_table->ept_pde[i][j], (i * 512) + j) == false)
					return false;
			}
		}

		return true;
	}

	auto setup_pml2_entries(__ept_state& ept_state, ept_pde_2mb& pde_entry, unsigned __int64 pfn) -> bool {
		pde_entry.page_frame_number = pfn;

		if (is_valid_for_large_page(pfn) == true) {
			pde_entry.memory_type = ept_get_memory_type(pfn, true);
			return true;
		} 
		else {
			void* split_buffer = ExAllocatePoolWithTag(NonPagedPool, sizeof(__ept_dynamic_split), VMM_POOL_TAG);
			if (split_buffer == nullptr) {
				LOG("Failed to allocate split buffer");
				LOG_ERROR(__FILE__, __LINE__);
				return false;
			}

			return split_pml2_entry(ept_state, split_buffer, pfn * LARGE_PAGE_SIZE);
		}
	}

	auto is_valid_for_large_page(unsigned __int64 pfn) -> bool {
		UNREFERENCED_PARAMETER(pfn);

		uint64_t page_start = pfn * LARGE_PAGE_SIZE;
		uint64_t page_end = (pfn * LARGE_PAGE_SIZE) + (LARGE_PAGE_SIZE - 1);

		//mtrr_entry* temp = reinterpret_cast<mtrr_entry*>(g_mtrr_entries);

		for (unsigned idx = 0; idx < g_vmm_context->mtrr_info.enabled_memory_ranges; idx++) {
			if (page_start <= g_vmm_context->mtrr_info.memory_range[idx].physcial_end_address &&
				page_end > g_vmm_context->mtrr_info.memory_range[idx].physcial_end_address)
				return false;

			else if (page_start < g_vmm_context->mtrr_info.memory_range[idx].physcial_base_address && 
				page_end >= g_vmm_context->mtrr_info.memory_range[idx].physcial_base_address)
				return false;
		}

		return true;
	}

	auto ept_get_memory_type(unsigned __int64 pfn, bool large_page) -> unsigned __int64 {
		unsigned __int64 page_start = large_page == true ? pfn * LARGE_PAGE_SIZE : pfn * PAGE_SIZE;
		unsigned __int64 page_end = large_page == true ? (pfn * LARGE_PAGE_SIZE) + (LARGE_PAGE_SIZE - 1) : (pfn * PAGE_SIZE) + (PAGE_SIZE - 1);
		unsigned __int64 memory_type = g_vmm_context->mtrr_info.default_memory_type;

		for (unsigned idx = 0; idx < g_vmm_context->mtrr_info.enabled_memory_ranges; idx++) {
			if (page_start >= g_vmm_context->mtrr_info.memory_range[idx].physcial_base_address && 
				page_end <= g_vmm_context->mtrr_info.memory_range[idx].physcial_end_address) {
				memory_type = g_vmm_context->mtrr_info.memory_range[idx].memory_type;

				if (g_vmm_context->mtrr_info.memory_range[idx].fixed_range == true)
					break;

				if (memory_type == Uncacheable)
					break;
			}
		}

		return memory_type;
	}

	auto split_pml2_entry(__ept_state& ept_state, void* buffer, unsigned __int64 physical_address) -> bool {
		ept_pde_2mb* entry = get_pde_entry(ept_state, physical_address);
		if (entry == nullptr) {
			LOG("[!] Invalid address passed");
			LOG_ERROR(__FILE__, __LINE__);
			return false;
		}

		struct _ept_split_page* new_split = (struct _ept_split_page*)buffer;
		RtlSecureZeroMemory(new_split, sizeof(struct _ept_split_page));

		//
		// Set all pages as rwx to prevent unwanted ept violation
		//
		new_split->ept_pde = entry;

		ept_pte entry_template = { 0 };
		entry_template.read_access = 1;
		entry_template.write_access = 1;
		entry_template.execute_access = 1;
		entry_template.memory_type = entry->memory_type;
		entry_template.ignore_pat = entry->ignore_pat;
		entry_template.suppress_ve = entry->suppress_ve;

		__stosq((unsigned __int64*)&new_split->ept_pte[0], entry_template.flags, 512);
		for (int i = 0; i < 512; i++) {
			unsigned __int64 pfn = ((entry->page_frame_number * LARGE_PAGE_SIZE) >> PAGE_SHIFT) + i;
			new_split->ept_pte[i].page_frame_number = pfn;
			new_split->ept_pte[i].memory_type = ept_get_memory_type(pfn, false);
		}

		ept_pde_2mb new_entry = { 0 };
		new_entry.read_access = 1;
		new_entry.write_access = 1;
		new_entry.execute_access = 1;

		new_entry.page_frame_number = MmGetPhysicalAddress(&new_split->ept_pte[0]).QuadPart >> PAGE_SHIFT;

		RtlCopyMemory(entry, &new_entry, sizeof(new_entry));

		return true;
	}

	auto get_pde_entry(__ept_state& ept_state, unsigned __int64 pfn) -> ept_pde_2mb* {
		unsigned __int64 pml4_index = MASK_EPT_PML4_INDEX(pfn);
		unsigned __int64 pml3_index = MASK_EPT_PML3_INDEX(pfn);
		unsigned __int64 pml2_index = MASK_EPT_PML2_INDEX(pfn);

		if (pml4_index > 0) {
			LOG("[!] Address above 512GB is invalid\n");
			LOG_ERROR(__FILE__, __LINE__);
			return nullptr;
		}

		return &ept_state.ept_page_table->ept_pde[pml3_index][pml2_index];
	}

	auto get_pte_entry(ept_page_table* page_table, unsigned __int64 pfn) -> ept_pte* {
		ept_pde_2mb* pde_entry = get_pde_entry(page_table, pfn);
		if (!pde_entry) {
			LOG("[!] Invalid pde address passed.\n");
			LOG_ERROR(__FILE__, __LINE__);
			return nullptr;
		}

		// Check to ensure the page is split
		if (pde_entry->large_page) {
			return nullptr;
		}

		ept_pte* pte_entry = reinterpret_cast<ept_pte*>
			(physical_to_virtual_address(pde_entry->page_frame_number << PAGE_SHIFT));
		if (!pte_entry)	return nullptr;

		uint64_t pte_index = MASK_EPT_PML1_INDEX(pfn);

		pte_entry = &pte_entry[pte_index];
		return pte_entry;
	}

	auto split_pde(ept_page_table* page_table, void* buffer, unsigned __int64 pfn) -> void {
		ept_pde_2mb* pde_entry = get_pde_entry(page_table, pfn);
		if (!pde_entry) {
			LOG("[!] Invalid pde address passed.\n");
			LOG_ERROR(__FILE__, __LINE__);
			return;
		}

		// If this large page is not marked a large page, that means it's a pointer already.
		// That page is therefore already split.
		if (!pde_entry->large_page) {
			return;
		}

		ept_split_page* split_page = reinterpret_cast<ept_split_page*>(buffer);
		RtlSecureZeroMemory(split_page, sizeof ept_split_page);

		// Set all pages as rwx to prevent unwanted ept violation
		split_page->ept_pde = pde_entry;

		ept_pte pte_template = { 0 };
		pte_template.read_access = 1;
		pte_template.write_access = 1;
		pte_template.execute_access = 1;
		pte_template.memory_type = pde_entry->memory_type;
		pte_template.ignore_pat = pde_entry->ignore_pat;
		pte_template.suppress_ve = pde_entry->suppress_ve;

		__stosq((SIZE_T*)&split_page->ept_pte[0], pte_template.flags, 512);
		for (unsigned idx = 0; idx < 512; idx++) {
			uint64_t page_number = ((pde_entry->page_frame_number * PAGE2MB) >> PAGE_SHIFT) + idx;
			split_page->ept_pte[idx].page_frame_number = page_number;
			//split_page->ept_pte[idx].memory_type = get_memory_type(page_number, FALSE);
		}

		ept_pde_2mb pde_2 = { 0 };
		pde_2.read_access = 1;
		pde_2.write_access = 1;
		pde_2.execute_access = 1;

		pde_2.page_frame_number = (virtual_to_physical_address((void*)&split_page->ept_pte[0]) >> PAGE_SHIFT);

		RtlCopyMemory(pde_entry, &pde_2, sizeof(pde_2));

		// Add our allocation to the linked list of dynamic splits for later deallocation 
		InsertHeadList((PLIST_ENTRY)&page_table->dynamic_pages, &split_page->split_pages);

		return;
	}

	auto ept_init_table_entry(ept_entry* _ept_entry, unsigned __int64 level, unsigned __int64 pfn) -> void {
		_ept_entry->read_access = 1;
		_ept_entry->write_access = 1;
		_ept_entry->execute_access = 1;
		_ept_entry->page_frame_number = pfn >> PAGE_SHIFT;

		if (level == 1) {
			_ept_entry->memory_type = ept_get_memory_type(pfn, false);
		}

		return;
	}

	auto ept_allocate_ept_entry(ept_page_table* page_table) -> ept_entry* {
		if (!page_table) {
			static const uint64_t kAllocSize = 512 * sizeof(ept_entry*);
			ept_entry* entry = reinterpret_cast<ept_entry*>
				(ExAllocatePoolZero(NonPagedPool, kAllocSize, VMM_POOL_TAG));
			if (!entry)
				return nullptr;

			RtlSecureZeroMemory(entry, kAllocSize);
			return entry;
		}

		else {
			const uint64_t count = InterlockedIncrement((LONG volatile*)&page_table->dynamic_pages_count);

			// How many EPT entries are preallocated. When the number exceeds it, return
			if (count > DYNAMICPAGESCOUNT) {
				return nullptr;
			}

			return page_table->dynamic_pages[count - 1];
		}
	}

	auto ept_inv_global_entry() -> unsigned __int64 {
		ept_error err = { 0 };
		return asm_inv_ept_global(invept_all_context, &err);
	}

	/*auto handle_ept_violation(unsigned __int64 phys_addr, unsigned __int64 linear_addr) -> bool {
		//ept_state* _ept_state = vmm_context[KeGetCurrentProcessorNumber()].ept_state;
		ept_state* _ept_state = (ept_state*)phys_addr;
		__debugbreak();

		// Get faulting page table entry (PTE)
		const ept_pte* pte_entry = get_pte_entry(_ept_state->ept_page_table, phys_addr);
		if (pte_entry && pte_entry->flags) {
			__debugbreak();
			LOG("[!][%ws] PteEntry: VA = %llx, PA = %llx", __FUNCTIONW__, linear_addr, phys_addr);
			LOG_ERROR(__FILE__, __LINE__);
			return false;
		}

		// EPT entry miss
		ept_entry* pml4e = reinterpret_cast<ept_entry*>
			(_ept_state->ept_page_table->ept_pml4);
		ept_construct_tables(pml4e, 4, phys_addr, _ept_state->ept_page_table);

		// invalidate Global EPT entries
		ept_inv_global_entry();

		return true;
	}*/

	auto ept_construct_tables(ept_entry* _ept_entry, unsigned __int64 level, unsigned __int64 pfn, ept_page_table* page_table) -> ept_entry* {
		switch (level) {
		case 4: {
			// table == PML4 (512 GB)
			const uint64_t pml4_index = MASK_EPT_PML4_INDEX(pfn);
			ept_entry* pml4_entry = &_ept_entry[pml4_index];
			LOG("[*] pml4_entry : %p\n", pml4_entry);
			if (!pml4_entry->flags) {
				ept_entry* ept_pdpt = reinterpret_cast<ept_entry*>
					(ept_allocate_ept_entry(page_table));
				if (!ept_pdpt)
					return nullptr;

				ept_init_table_entry(pml4_entry, level, virtual_to_physical_address(ept_pdpt));
			}

			ept_entry* temp = reinterpret_cast<ept_entry*>
				(physical_to_virtual_address(pml4_entry->page_frame_number << PAGE_SHIFT));
			return ept_construct_tables(temp, level - 1, pfn, page_table);
		}

		case 3: {
			// table == PDPT (1 GB)
			const uint64_t pml3_index = MASK_EPT_PML3_INDEX(pfn);
			ept_entry* pdpt_entry = &_ept_entry[pml3_index];
			LOG("[*] pdpt_entry : %p\n", pdpt_entry);

			// ---------------------------------
			/*if (!pdpt_entry->read_access)
				pdpt_entry->read_access = 0x1;
			if (!pdpt_entry->write_access)
				pdpt_entry->write_access = 0x1;
			if (!pdpt_entry->execute_access)
				pdpt_entry->execute_access = 0x1;*/
			// ----------------------------------

			if (!pdpt_entry->flags) {
				ept_entry* _ept_pde = reinterpret_cast<ept_entry*>
					(ept_allocate_ept_entry(page_table));
				if (!_ept_pde)
					return nullptr;

				ept_init_table_entry(pdpt_entry, level, _ept_pde->flags);
			}

			ept_entry* temp = reinterpret_cast<ept_entry*>
				(physical_to_virtual_address(pdpt_entry->page_frame_number << PAGE_SHIFT));
			return ept_construct_tables(temp, level - 1, pfn, page_table);
		}

		case 2: {
			// table == PDT (2 MB)
			const uint64_t pml2_index = MASK_EPT_PML2_INDEX(pfn);
			ept_entry* pde_entry = &_ept_entry[pml2_index];
			LOG("[*] pde_entry : %p\n", pde_entry);
			if (pde_entry == nullptr) {
				ept_entry* ept_pt = ept_allocate_ept_entry(page_table);
				LOG("[*] ept entry : %p\n", static_cast<void*>(ept_pt));
				return nullptr;
			}
			if (!pde_entry->flags) {
				ept_entry* ept_pt = ept_allocate_ept_entry(page_table);
				if (!ept_pt)
					return nullptr;

				ept_init_table_entry(pde_entry, level, ept_pt->flags);
			}

			ept_entry* temp = reinterpret_cast<ept_entry*>
				(physical_to_virtual_address(pde_entry->page_frame_number << PAGE_SHIFT));
			return ept_construct_tables(temp, level - 1, pfn, page_table);
		}

		case 1: {
			// table == PT (4 KB)
			const uint64_t pte_index = (pfn >> 12) & 0x1ff; //MASK_EPT_PML1_INDEX(pfn);
			ept_entry* pte_entry = &_ept_entry[pte_index];
			NT_ASSERT(!pte_entry->AsUInt);
			ept_init_table_entry(pte_entry, level, pfn);
			return pte_entry;
		}

		default:
			__debugbreak();
			return nullptr;
		}
	}
}
