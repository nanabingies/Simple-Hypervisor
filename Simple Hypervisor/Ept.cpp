#include "ept.hpp"
#pragma warning(disable: 4996)

namespace ept {
	static mtrr_entry g_mtrr_entries[num_mtrr_entries];

	auto check_ept_support() -> bool {
		PAGED_CODE();

		PROCESSOR_NUMBER processor_number;
		GROUP_AFFINITY affinity, old_affinity;

		for (unsigned iter = 0; iter < g_num_processors; iter++) {
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

	auto IsInRange(unsigned __int64 val, unsigned __int64 start, unsigned __int64 end) -> bool {
		if (val > start && val <= end)
			return true;

		return false;
	}

	auto ept_build_mtrr_map() -> bool {
		PAGED_CODE();

		mtrr_entry* _mtrr_entry = g_mtrr_entries;
		RtlSecureZeroMemory(_mtrr_entry, num_mtrr_entries * sizeof(mtrr_entry));

		ia32_mtrr_capabilities_register mtrr_cap;
		ia32_mtrr_def_type_register mtrr_def;
		ia32_mtrr_physbase_register mtrr_phys_base;
		ia32_mtrr_physmask_register mtrr_phys_mask;

		mtrr_cap.flags = __readmsr(IA32_MTRR_CAPABILITIES);
		mtrr_def.flags = __readmsr(IA32_MTRR_DEF_TYPE);

		uint64_t varCnt = mtrr_cap.variable_range_count;
		uint64_t FixRangeSupport = mtrr_cap.fixed_range_supported;
		uint64_t FixRangeEnable = mtrr_def.fixed_range_mtrr_enable;
		g_default_memory_type = mtrr_def.default_memory_type;

		if (FixRangeSupport && FixRangeEnable) {
			// Handle Fix Ranged MTRR

			static const uint64_t k64kBase = IA32_MTRR_FIX64K_BASE;
			static const uint64_t k64kManagedSize = IA32_MTRR_FIX64K_SIZE;	// 64K
			static const uint64_t k16kBase = IA32_MTRR_FIX16K_BASE;
			static const uint64_t k16kManagedSize = IA32_MTRR_FIX16K_SIZE;
			static const uint64_t k4kBase = IA32_MTRR_FIX4K_BASE;
			static const uint64_t k4kManagedSize = IA32_MTRR_FIX4K_SIZE;

			uint64_t offset = 0x0;

			// Let's first set Fix64K MTRR
			ia32_mtrr_fixed_range_msr msr64k;
			msr64k.all = __readmsr(IA32_MTRR_FIX64K_00000);
			for (unsigned idx = 0; idx < 8; idx++) {
				UINT64 base = k64kBase + offset;
				offset += k64kManagedSize;

				// Save the MTRR
				_mtrr_entry->mtrr_enabled = true;
				_mtrr_entry->memory_type = msr64k.fields.types[idx];
				_mtrr_entry->mtrr_fixed = true;
				_mtrr_entry->physical_address_start = base;
				_mtrr_entry->physical_address_end = base + k64kManagedSize - 1;

				g_mtrr_num += 1;
				_mtrr_entry++;
			}


			// let's set Fix16k MTRR
			ia32_mtrr_fixed_range_msr msr16k;
			// start -- IA32_MTRR_FIX16K_80000	
			// end   -- IA32_MTRR_FIX16K_A0000
			offset = 0;

			for (unsigned start = IA32_MTRR_FIX16K_80000; start <= IA32_MTRR_FIX16K_A0000; start++) {
				msr16k.all = __readmsr(start);
				for (unsigned idx = 0; idx < 8; idx++) {
					uint64_t base = k16kBase + offset;
					offset += k16kManagedSize;

					// Save the MTRR
					_mtrr_entry->memory_type = msr16k.fields.types[idx];
					_mtrr_entry->mtrr_enabled = true;
					_mtrr_entry->mtrr_fixed = true;
					_mtrr_entry->physical_address_start = base;
					_mtrr_entry->physical_address_end = base + k16kManagedSize - 1;

					g_mtrr_num += 1;
					_mtrr_entry++;
				}
			}


			// let's set Fix4k MTRR
			ia32_mtrr_fixed_range_msr msr4k;
			// start -- IA32_MTRR_FIX4K_C0000	
			// end   -- IA32_MTRR_FIX4K_F8000
			offset = 0;

			for (unsigned start = IA32_MTRR_FIX4K_C0000; start <= IA32_MTRR_FIX4K_F8000; start++) {
				msr4k.all = __readmsr(start);
				for (unsigned idx = 0; idx < 8; idx++) {
					uint64_t base = k4kBase + offset;
					offset += k4kManagedSize;

					// Save the MTRR
					_mtrr_entry->memory_type = msr4k.fields.types[idx];
					_mtrr_entry->mtrr_enabled = true;
					_mtrr_entry->mtrr_fixed = true;
					_mtrr_entry->physical_address_start = base;
					_mtrr_entry->physical_address_end = base + k4kManagedSize - 1;

					_mtrr_entry++;
				}
			}
		}

		for (unsigned iter = 0; iter < varCnt; iter++) {
			mtrr_phys_base.flags = __readmsr(IA32_MTRR_PHYSBASE0 + (iter * 2));
			mtrr_phys_mask.flags = __readmsr(IA32_MTRR_PHYSMASK0 + (iter * 2));

			// The range is invalid
			if (!mtrr_phys_mask.valid)
				continue;

			// Get the length this MTRR manages
			ULONG length;
			BitScanForward64(&length, mtrr_phys_mask.page_frame_number * PAGE_SIZE);

			uint64_t physical_base_start, physical_base_end;
			physical_base_start = mtrr_phys_base.page_frame_number * PAGE_SIZE;
			physical_base_end = physical_base_start + ((1ull << length) - 1);

			_mtrr_entry->mtrr_enabled = true;
			_mtrr_entry->mtrr_fixed = false;
			_mtrr_entry->memory_type = mtrr_phys_base.type;
			_mtrr_entry->physical_address_start = physical_base_start;
			_mtrr_entry->physical_address_end = physical_base_end;
			g_mtrr_num++;
			_mtrr_entry++;
		}

		return true;
	}

	auto initialize_ept(unsigned char processor_number) -> bool {
		PAGED_CODE();

		ept_state* _ept_state = reinterpret_cast<ept_state*>
			(ExAllocatePoolWithTag(NonPagedPool, sizeof(ept_state), VMM_POOL_TAG));
		if (!_ept_state) {
			LOG("[-] Failed to allocate memory for Ept State.\n");
			LOG_ERROR();
			return false;
		}
		RtlSecureZeroMemory(_ept_state, sizeof ept_state);

		ept_pointer* ept_ptr = reinterpret_cast<ept_pointer*>
			(ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, VMM_POOL_TAG));
		if (!ept_ptr) {
			LOG("[-] Failed to allocate memory for pointer to Ept.\n");
			LOG_ERROR();
			return false;
		}
		_ept_state->ept_ptr = ept_ptr;
		RtlSecureZeroMemory(ept_ptr, PAGE_SIZE);


		if (create_ept_state(_ept_state) == false) {
			DbgPrint("[-] Failed to set Ept Page Table Entries.\n");
			LOG_ERROR();
			ExFreePoolWithTag(ept_ptr, VMM_POOL_TAG);
			ExFreePoolWithTag(_ept_state, VMM_POOL_TAG);
			return false;
		}

		vmm_context[processor_number].ept_ptr = _ept_state->ept_ptr->flags;
		vmm_context[processor_number].ept_state = _ept_state;


		LOG("[*] EPT initialized on processor (%x)\n", processor_number);
		return true;
	}

	auto create_ept_state(ept_state* _ept_state) -> bool {
		ept_page_table* page_table = reinterpret_cast<ept_page_table*>
			(ExAllocatePoolWithTag(NonPagedPool, sizeof ept_page_table, VMM_POOL_TAG));
		if (!page_table)	return false;
		_ept_state->ept_page_table = page_table;

		ept_pml4e* pml4e = reinterpret_cast<ept_pml4e*>(&page_table->ept_pml4[0]);
		ept_pdpte* pdpte = reinterpret_cast<ept_pdpte*>(&page_table->ept_pdpte[0]);

		_ept_state->ept_ptr->page_frame_number = (virtual_to_physical_address(&pml4e) >> PAGE_SHIFT);
		_ept_state->ept_ptr->enable_access_and_dirty_flags = 0;
		_ept_state->ept_ptr->memory_type = WriteBack;
		_ept_state->ept_ptr->page_walk_length = max_ept_walk_length - 1;

		vmm_context[KeGetCurrentProcessorNumber()].ept_pml4 = pml4e->flags;

		pml4e->page_frame_number = (virtual_to_physical_address(&pdpte) >> PAGE_SHIFT);
		pml4e->execute_access = 1;
		pml4e->read_access = 1;
		pml4e->user_mode_execute = 1;
		pml4e->write_access = 1;

		ept_pdpte pdpte_template = { 0 };
		pdpte_template.read_access = 1;
		pdpte_template.write_access = 1;
		pdpte_template.execute_access = 1;

		__stosq((SIZE_T*)&page_table->ept_pdpte[0], pdpte_template.flags, EPTPDPTEENTRIES);
		for (unsigned idx = 0; idx < EPTPDPTEENTRIES; idx++) {
			page_table->ept_pdpte[idx].page_frame_number = (virtual_to_physical_address(&page_table->ept_pde[idx][0]) >> PAGE_SHIFT);
		}

		ept_pde_2mb pde_template = { 0 };
		pde_template.read_access = 1;
		pde_template.write_access = 1;
		pde_template.execute_access = 1;
		pde_template.large_page = 1;

		__stosq((SIZE_T*)&page_table->ept_pde[0], pde_template.flags, EPTPDEENTRIES);
		for (unsigned i = 0; i < EPTPML4ENTRIES; i++) {
			for (unsigned j = 0; j < EPTPDPTEENTRIES; j++) {
				setup_pml2_entries(_ept_state, &page_table->ept_pde[i][j], (i * 512) + j);
			}
		}

		// Allocate preallocated entries
		const uint64_t preallocated_entries_size = sizeof(ept_entry) * DYNAMICPAGESCOUNT;
		/*const*/ ept_entry** dynamic_pages = reinterpret_cast<ept_entry**>
			(ExAllocatePoolWithTag(NonPagedPool, preallocated_entries_size, VMM_POOL_TAG));
		if (!dynamic_pages) {
			ExFreePoolWithTag(page_table, VMM_POOL_TAG);
			return FALSE;
		}
		RtlSecureZeroMemory(dynamic_pages, preallocated_entries_size);

		/*const*/ ept_entry * _ept_entry = ept_allocate_ept_entry(NULL);
		if (!_ept_entry) {
			ExFreePoolWithTag(dynamic_pages, VMM_POOL_TAG);
			ExFreePoolWithTag(page_table, VMM_POOL_TAG);
			return FALSE;
		}
		dynamic_pages[0] = _ept_entry;

		page_table->dynamic_pages_count = 0;
		page_table->dynamic_pages = const_cast<ept_entry**>(dynamic_pages);
		_ept_state->guest_address_width_value = max_ept_walk_length - 1;

		return true;
	}

	auto setup_pml2_entries(ept_state* _ept_state, ept_pde_2mb* pde_entry, unsigned __int64 pfn) -> void {
		UNREFERENCED_PARAMETER(_ept_state);

		pde_entry->page_frame_number = pfn;

		uint64_t addrOfPage = pfn * PAGE2MB;
		if (pfn == 0) {
			pde_entry->memory_type = Uncacheable;
			return;
		}

		uint64_t memory_type = WriteBack;
		mtrr_entry* temp = reinterpret_cast<mtrr_entry*>(g_mtrr_entries);
		for (unsigned idx = 0; idx < g_mtrr_num; idx++) {
			if (addrOfPage <= temp[idx].physical_address_end) {
				if ((addrOfPage + PAGE2MB - 1) >= temp[idx].physical_address_start) {
					memory_type = temp[idx].memory_type;
					if (memory_type == Uncacheable) {
						break;
					}
				}
			}
		}

		pde_entry->memory_type = memory_type;

		return;
	}

	auto is_valid_for_large_page(unsigned __int64 pfn) -> bool {
		UNREFERENCED_PARAMETER(pfn);

		uint64_t page_start = pfn * PAGE2MB;
		uint64_t page_end = (pfn * PAGE2MB) + (PAGE2MB - 1);

		mtrr_entry* temp = reinterpret_cast<mtrr_entry*>(g_mtrr_entries);

		for (unsigned idx = 0; idx < g_mtrr_num; idx++) {
			if (page_start <= temp[idx].physical_address_end && page_end > temp[idx].physical_address_end)
				return false;

			else if (page_start < temp[idx].physical_address_start && page_end >= temp[idx].physical_address_start)
				return false;
		}

		return true;
	}

	auto ept_get_memory_type(unsigned __int64 pfn, bool large_page) -> unsigned __int64 {
		uint64_t page_start = large_page == true ? pfn * PAGE2MB : pfn * PAGE_SIZE;
		uint64_t page_end = large_page == true ? (pfn * PAGE2MB) + (PAGE2MB - 1) : (pfn * PAGE_SIZE) + (PAGE_SIZE - 1);
		uint64_t memory_type = g_default_memory_type;

		mtrr_entry* temp = reinterpret_cast<mtrr_entry*>(g_mtrr_entries);

		for (unsigned idx = 0; idx < g_mtrr_num; idx++) {
			if (page_start >= temp[idx].physical_address_start && page_end <= temp[idx].physical_address_end) {
				memory_type = temp[idx].memory_type;

				if (temp[idx].mtrr_fixed == true)
					break;

				if (memory_type == Uncacheable)
					break;
			}
		}

		return memory_type;
	}

	auto get_pde_entry(ept_page_table* page_table, unsigned __int64 pfn) -> ept_pde_2mb* {
		uint64_t pml4_index = MASK_EPT_PML4_INDEX(pfn);
		uint64_t pml3_index = MASK_EPT_PML3_INDEX(pfn);
		uint64_t pml2_index = MASK_EPT_PML2_INDEX(pfn);

		if (pml4_index > 0) {
			LOG("Address above 512GB is invalid\n");
			LOG_ERROR();
			return nullptr;
		}

		return &page_table->ept_pde[pml3_index][pml2_index];
	}

	auto get_pte_entry(ept_page_table* page_table, unsigned __int64 pfn) -> ept_pte* {
		ept_pde_2mb* pde_entry = get_pde_entry(page_table, pfn);
		if (!pde_entry) {
			LOG("[-] Invalid pde address passed.\n");
			LOG_ERROR();
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
			LOG("[-] Invalid pde address passed.\n");
			LOG_ERROR();
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

	auto handle_ept_violation(unsigned __int64 phys_addr, unsigned __int64 linear_addr) -> void {
		ept_state* _ept_state = vmm_context[KeGetCurrentProcessorNumber()].ept_state;

		// Get faulting page table entry (PTE)
		const ept_pte* pte_entry = get_pte_entry(_ept_state->ept_page_table, phys_addr);
		if (pte_entry && pte_entry->flags) {
			__debugbreak();
			LOG("PteEntry: VA = %llx, PA = %llx", linear_addr, phys_addr);
			return;
		}

		// EPT entry miss
		ept_entry* pml4e = reinterpret_cast<ept_entry*>
			(_ept_state->ept_page_table->ept_pml4);
		ept_construct_tables(pml4e, 4, phys_addr, _ept_state->ept_page_table);

		// invalidate Global EPT entries
		ept_inv_global_entry();

		return;
	}

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
