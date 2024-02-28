#include "stdafx.h"
#pragma warning(disable : 4996)

static MtrrEntry g_MtrrEntries[numMtrrEntries];

auto IsInRange(uint64_t val, uint64_t start, uint64_t end) -> bool {
	if (val > start && val <= end)
		return true;

	return false;
}

auto CheckEPTSupport() -> bool {
	PAGED_CODE();

	ia32_vmx_ept_vpid_cap_register ept_cap;
	ept_cap.flags = __readmsr(IA32_VMX_EPT_VPID_CAP);

	if (!ept_cap.page_walk_length_4 || !ept_cap.memory_type_write_back || !ept_cap.invept ||
		!ept_cap.invept_single_context || !ept_cap.invept_all_contexts || !ept_cap.invvpid ||
		!ept_cap.invvpid_individual_address || !ept_cap.invvpid_all_contexts ||
		!ept_cap.invvpid_single_context || !ept_cap.invvpid_single_context_retain_globals) {
		return false;
	}

	return true;
}

auto EptBuildMTRRMap() -> bool {
	PAGED_CODE();

	MtrrEntry* mtrr_entry = g_MtrrEntries;
	RtlSecureZeroMemory(mtrr_entry, numMtrrEntries * sizeof(struct _MtrrEntry));
	
	ia32_mtrr_capabilities_register mtrr_cap;
	ia32_mtrr_def_type_register mtrr_def;
	ia32_mtrr_physbase_register mtrr_phys_base;
	ia32_mtrr_physmask_register mtrr_phys_mask;
	
	mtrr_cap.flags = __readmsr(IA32_MTRR_CAPABILITIES);
	mtrr_def.flags = __readmsr(IA32_MTRR_DEF_TYPE);

	uint64_t varCnt = mtrr_cap.variable_range_count;
	uint64_t FixRangeSupport = mtrr_cap.fixed_range_supported;
	uint64_t FixRangeEnable = mtrr_def.fixed_range_mtrr_enable;
	g_DefaultMemoryType = mtrr_def.default_memory_type;

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
		Ia32MtrrFixedRangeMsr msr64k;
		msr64k.all = __readmsr(IA32_MTRR_FIX64K_00000);
		for (unsigned idx = 0; idx < 8; idx++) {
			uint64_t base = k64kBase + offset;
			offset += k64kManagedSize;

			// Save the MTRR
			mtrr_entry->MtrrEnabled = true;
			mtrr_entry->MemoryType = msr64k.fields.types[idx];
			mtrr_entry->MtrrFixed = true;
			mtrr_entry->PhysicalAddressStart = base;
			mtrr_entry->PhysicalAddressEnd = base + k64kManagedSize - 1;

			gMtrrNum += 1;
			mtrr_entry++;
		}

		
		// let's set Fix16k MTRR
		Ia32MtrrFixedRangeMsr msr16k;
		// start -- IA32_MTRR_FIX16K_80000	
		// end   -- IA32_MTRR_FIX16K_A0000
		offset = 0;

		for (unsigned start = IA32_MTRR_FIX16K_80000; start <= IA32_MTRR_FIX16K_A0000; start++) {
			msr16k.all = __readmsr(start);
			for (unsigned idx = 0; idx < 8; idx++) {
				uint64_t base = k16kBase + offset;
				offset += k16kManagedSize;

				// Save the MTRR
				mtrr_entry->MemoryType = msr16k.fields.types[idx];
				mtrr_entry->MtrrEnabled = true;
				mtrr_entry->MtrrFixed = true;
				mtrr_entry->PhysicalAddressStart = base;
				mtrr_entry->PhysicalAddressEnd = base + k16kManagedSize - 1;

				gMtrrNum += 1;
				mtrr_entry++;
			}
		}

		
		// let's set Fix4k MTRR
		Ia32MtrrFixedRangeMsr msr4k;
		// start -- IA32_MTRR_FIX4K_C0000	
		// end   -- IA32_MTRR_FIX4K_F8000
		offset = 0;

		for (unsigned start = IA32_MTRR_FIX4K_C0000; start <= IA32_MTRR_FIX4K_F8000; start++) {
			msr4k.all = __readmsr(start);
			for (unsigned idx = 0; idx < 8; idx++) {
				uint64_t base = k4kBase + offset;
				offset += k4kManagedSize;

				// Save the MTRR
				mtrr_entry->MemoryType = msr4k.fields.types[idx];
				mtrr_entry->MtrrEnabled = true;
				mtrr_entry->MtrrFixed = true;
				mtrr_entry->PhysicalAddressStart = base;
				mtrr_entry->PhysicalAddressEnd = base + k4kManagedSize - 1;

				mtrr_entry++;
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
		ulong length;
		BitScanForward64(&length, mtrr_phys_mask.page_frame_number * PAGE_SIZE);

		uint64_t physical_base_start, physical_base_end;
		physical_base_start = mtrr_phys_base.page_frame_number * PAGE_SIZE;
		physical_base_end = physical_base_start + ((1ull << length) - 1);

		mtrr_entry->MtrrEnabled = true;
		mtrr_entry->MtrrFixed = false;
		mtrr_entry->MemoryType = mtrr_phys_base.type;
		mtrr_entry->PhysicalAddressStart = physical_base_start;
		mtrr_entry->PhysicalAddressEnd = physical_base_end;
		gMtrrNum++;
		mtrr_entry++;
	}

	return TRUE;
}

auto InitializeEpt(uchar processorNumber) -> bool {
	PAGED_CODE();

	EptState* ept_state = reinterpret_cast<EptState*>
		(ExAllocatePoolWithTag(NonPagedPool, sizeof(_EptState), VMM_POOL));
	if (!ept_state) {
		DbgPrint("[-] Failed to allocate memory for Ept State.\n");
		return false;
	}
	RtlSecureZeroMemory(ept_state, sizeof(_EptState));

	ept_pointer* ept_ptr = reinterpret_cast<ept_pointer*>
		(ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, VMM_POOL));
	if (!ept_ptr) {
		DbgPrint("[-] Failed to allocate memory for pointer to Ept.\n");
		return false;
	}
	ept_state->EptPtr = ept_ptr;
	RtlSecureZeroMemory(ept_ptr, PAGE_SIZE);

	
	if (!CreateEptState(ept_state)) {
		DbgPrint("[-] Failed to set Ept Page Table Entries.\n");
		ExFreePoolWithTag(ept_ptr, VMM_POOL);
		ExFreePoolWithTag(ept_state, VMM_POOL);
		return false;
	}
	
	vmm_context[processorNumber].EptPtr = ept_state->EptPtr->flags;
	vmm_context[processorNumber].EptState = ept_state;

	
	DbgPrint("[*] EPT initialized.\n");
	return true;
}

auto CreateEptState(EptState* ept_state) -> bool {
	
	EptPageTable* page_table = reinterpret_cast<EptPageTable*>
		(ExAllocatePoolWithTag(NonPagedPool, sizeof(_EptPageTable), VMM_POOL));
	if (!page_table)	return FALSE;
	ept_state->EptPageTable = page_table;

	ept_pml4e* pml4e = reinterpret_cast<ept_pml4e*>(&page_table->EptPml4[0]);
	ept_pdpte* pdpte = reinterpret_cast<ept_pdpte*>(&page_table->EptPdpte[0]);
	
	ept_state->EptPtr->page_frame_number = (VirtualToPhysicalAddress(&pml4e) >> PAGE_SHIFT);
	ept_state->EptPtr->enable_access_and_dirty_flags = 0;
	ept_state->EptPtr->memory_type = WriteBack;
	ept_state->EptPtr->page_walk_length = MaxEptWalkLength - 1;

	vmm_context[KeGetCurrentProcessorNumber()].EptPml4 = pml4e->flags;

	pml4e->page_frame_number = (VirtualToPhysicalAddress(&pdpte) >> PAGE_SHIFT);
	pml4e->execute_access = 1;
	pml4e->read_access = 1;
	pml4e->user_mode_execute = 1;
	pml4e->write_access = 1;

	ept_pdpte pdpte_template = { 0 };
	pdpte_template.read_access = 1;
	pdpte_template.write_access = 1;
	pdpte_template.execute_access = 1;

	__stosq(reinterpret_cast<SIZE_T*>(&page_table->EptPdpte[0]), pdpte_template.flags, EPTPDPTEENTRIES);
	for (unsigned idx = 0; idx < EPTPDPTEENTRIES; idx++) {
		page_table->EptPdpte[idx].page_frame_number = (VirtualToPhysicalAddress(&page_table->EptPde[idx][0]) >> PAGE_SHIFT);
	}

	ept_pde_2mb pde_template = { 0 };
	pde_template.read_access = 1;
	pde_template.write_access = 1;
	pde_template.execute_access = 1;
	pde_template.large_page = 1;
	
	__stosq(reinterpret_cast<SIZE_T*>(&page_table->EptPde[0]), pde_template.flags, EPTPDEENTRIES);
	for (unsigned i = 0; i < EPTPML4ENTRIES; i++) {
		for (unsigned j = 0; j < EPTPDPTEENTRIES; j++) {
			SetupPml2Entries(ept_state, &page_table->EptPde[i][j], (i * 512) + j);
		}
	}

	// Allocate preallocated entries
	const uint64_t preallocated_entries_size = sizeof(ept_entry) * DYNAMICPAGESCOUNT;
	const ept_entry** dynamic_pages = reinterpret_cast<ept_entry**>
		(ExAllocatePoolWithTag(NonPagedPool, preallocated_entries_size, VMM_POOL));
	if (!dynamic_pages) {
		ExFreePoolWithTag(page_table, VMM_POOL);
		return false;
	}
	RtlSecureZeroMemory(dynamic_pages, preallocated_entries_size);

	const ept_entry* _ept_entry = EptAllocateEptEntry(NULL);
	if (!_ept_entry) {
		ExFreePoolWithTag(dynamic_pages, VMM_POOL);
		ExFreePoolWithTag(page_table, VMM_POOL);
		return FALSE;
	}
	dynamic_pages[0] = _ept_entry;

	page_table->DynamicPagesCount = 0;
	page_table->DynamicPages = dynamic_pages;
	ept_state->GuestAddressWidthValue = MaxEptWalkLength - 1;
	return TRUE;
}

auto SetupPml2Entries(EptState* ept_state, ept_pde_2mb* pde_entry, uint64_t pfn) -> void {
	UNREFERENCED_PARAMETER(ept_state);

	pde_entry->page_frame_number = pfn;

	UINT64 addrOfPage = pfn * PAGE2MB;
	if (pfn == 0) {
		pde_entry->memory_type = Uncacheable;
		return;
	}

	UINT64 memory_type = WriteBack;
	MtrrEntry* temp = (MtrrEntry*)g_MtrrEntries;
	for (unsigned idx = 0; idx < gMtrrNum; idx++) {
		if (addrOfPage <= temp[idx].PhysicalAddressEnd) {
			if ((addrOfPage + PAGE2MB - 1) >= temp[idx].PhysicalAddressStart) {
				memory_type = temp[idx].MemoryType;
				if (memory_type == Uncacheable) {
					break;
				}
			}
		}
	}

	pde_entry->memory_type = memory_type;

	return;
}

auto IsValidForLargePage(uint64_t pfn) -> bool {
	UNREFERENCED_PARAMETER(pfn);

	uint64_t page_start = pfn * PAGE2MB;
	uint64_t page_end = (pfn * PAGE2MB) + (PAGE2MB - 1);

	MtrrEntry* temp = reinterpret_cast<MtrrEntry*>(g_MtrrEntries);

	for (unsigned idx = 0; idx < gMtrrNum; idx++) {
		if (page_start <= temp[idx].PhysicalAddressEnd && page_end > temp[idx].PhysicalAddressEnd)
			return false;

		else if (page_start < temp[idx].PhysicalAddressStart && page_end >= temp[idx].PhysicalAddressStart)
			return false;
	}

	return true;
}

auto EptGetMemoryType(uint64_t pfn, bool large_page) -> uint64_t {
	uint64_t page_start = large_page == TRUE ? pfn * PAGE2MB : pfn * PAGE_SIZE;
	uint64_t page_end = large_page == TRUE ? (pfn * PAGE2MB) + (PAGE2MB - 1) : (pfn * PAGE_SIZE) + (PAGE_SIZE - 1);
	uint64_t memory_type = g_DefaultMemoryType;

	MtrrEntry* temp = reinterpret_cast<MtrrEntry*>(g_MtrrEntries);

	for (unsigned idx = 0; idx < gMtrrNum; idx++) {
		if (page_start >= temp[idx].PhysicalAddressStart && page_end <= temp[idx].PhysicalAddressEnd) {
			memory_type = temp[idx].MemoryType;

			if (temp[idx].MtrrFixed == TRUE)
				break;

			if (memory_type == Uncacheable)
				break;
		}
	}

	return memory_type;
}

auto GetPdeEntry(EptPageTable* page_table, uint64_t pfn) -> ept_pde_2mb* {
	UINT64 pml4_index = MASK_EPT_PML4_INDEX(pfn);
	UINT64 pml3_index = MASK_EPT_PML3_INDEX(pfn);
	UINT64 pml2_index = MASK_EPT_PML2_INDEX(pfn);

	if (pml4_index > 0) {
		DbgPrint("Address above 512GB is invalid\n");
		return NULL;
	}
	
	return &page_table->EptPde[pml3_index][pml2_index];
}

EPT_PTE* GetPteEntry(EptPageTable* page_table, UINT64 pfn) {
	EPT_PDE_2MB* pde_entry = GetPdeEntry(page_table, pfn);
	if (pde_entry == NULL) {
		DbgPrint("[-] Invalid pde address passed.\n");
		return NULL;
	}

	// Check to ensure the page is split
	if (pde_entry->LargePage) {
		return NULL;
	}

	EPT_PTE* pte_entry = (EPT_PTE*)PhysicalToVirtualAddress(pde_entry->PageFrameNumber << PAGE_SHIFT);
	if (pte_entry == NULL)	return NULL;

	UINT64 pte_index = MASK_EPT_PML1_INDEX(pfn);

	pte_entry = &pte_entry[pte_index];
	return pte_entry;
}

VOID SplitPde(EptPageTable* page_table, PVOID buffer, UINT64 pfn) {
	EPT_PDE_2MB* pde_entry = GetPdeEntry(page_table, pfn);
	if (pde_entry == NULL) {
		DbgPrint("[-] Invalid pde address passed.\n");
		return;
	}

	// If this large page is not marked a large page, that means it's a pointer already.
	// That page is therefore already split.
	if (!pde_entry->LargePage) {
		return;
	}

	EptSplitPage* split_page = (EptSplitPage*)buffer;
	RtlSecureZeroMemory(split_page, sizeof(struct _EptSplitPage));

	// Set all pages as rwx to prevent unwanted ept violation
	split_page->EptPde = pde_entry;

	EPT_PTE entry_template = { 0 };
	entry_template.ReadAccess = 1;
	entry_template.WriteAccess = 1;
	entry_template.ExecuteAccess = 1;
	entry_template.MemoryType = pde_entry->MemoryType;
	entry_template.IgnorePat = pde_entry->IgnorePat;
	entry_template.SuppressVe = pde_entry->SuppressVe;

	__stosq((SIZE_T*) & split_page->EptPte[0], entry_template.AsUInt, 512);
	for (unsigned idx = 0; idx < 512; idx++) {
		UINT64 page_number = ((pde_entry->PageFrameNumber * PAGE2MB) >> PAGE_SHIFT) + idx;
		split_page->EptPte[idx].PageFrameNumber = page_number;
		//split_page->EptPte[idx].MemoryType = GetMemoryType(page_number, FALSE);
	}

	EPT_PDE_2MB pde_2 = { 0 };
	pde_2.ReadAccess = 1;
	pde_2.WriteAccess = 1;
	pde_2.ExecuteAccess = 1;

	pde_2.PageFrameNumber = (VirtualToPhysicalAddress((void*) & split_page->EptPte[0]) >> PAGE_SHIFT);

	RtlCopyMemory(pde_entry, &pde_2, sizeof(pde_2));

	// Add our allocation to the linked list of dynamic splits for later deallocation 
	InsertHeadList((PLIST_ENTRY) & page_table->DynamicPages, &split_page->SplitPages);

	return;
}

VOID EptInitTableEntry(EPT_ENTRY* ept_entry, UINT64 level, UINT64 pfn) {
	ept_entry->ReadAccess = 1;
	ept_entry->WriteAccess = 1;
	ept_entry->ExecuteAccess = 1;
	ept_entry->PageFrameNumber = pfn >> PAGE_SHIFT;

	if (level == 1) {
		ept_entry->MemoryType = EptGetMemoryType(pfn, FALSE);
	}

	return;
}

auto EptAllocateEptEntry(EptPageTable* page_table) -> ept_entry* {
	if (page_table == NULL) {
		static const uint64_t kAllocSize = 512 * sizeof(ept_entry*);
		ept_entry* entry = reinterpret_cast<ept_entry*>
			(ExAllocatePoolZero(NonPagedPool, kAllocSize, VMM_POOL));
		if (!entry)
			return NULL;

		RtlSecureZeroMemory(entry, kAllocSize);
		return entry;
	}

	else {
		const UINT64 count = InterlockedIncrement((LONG volatile*) & page_table->DynamicPagesCount);

		// How many EPT entries are preallocated. When the number exceeds it, return
		if (count > DYNAMICPAGESCOUNT) {
			return NULL;
		}

		return page_table->DynamicPages[count - 1];
	}
}

UINT64 EptInvGlobalEntry() {
	ept_err err = { 0 };
	return AsmInveptGlobal(InveptAllContext, &err);
}

VOID HandleEptViolation(UINT64 phys_addr, UINT64 linear_addr) {
	EptState* ept_state = vmm_context[KeGetCurrentProcessorNumber()].EptState;

	// Get faulting page table entry (PTE)
	const EPT_PTE* pte_entry = GetPteEntry(ept_state->EptPageTable, phys_addr);
	if (pte_entry && pte_entry->AsUInt) {
		__debugbreak();
		DbgPrint("PteEntry: VA = %llx, PA = %llx", linear_addr, phys_addr);
		return;
	}

	// EPT entry miss
	//EPT_ENTRY* pml4e = (EPT_ENTRY*)ept_state->EptPageTable->EptPml4;
	//EptConstructTables(pml4e, 4, phys_addr, ept_state->EptPageTable);

	// invalidate Global EPT entries
	EptInvGlobalEntry();

	return;
}

const EPT_ENTRY* EptConstructTables(EPT_ENTRY* ept_entry, UINT64 level, UINT64 pfn, EptPageTable* page_table) {
	switch (level) {
	case 4: {
		// table == PML4 (512 GB)
		const UINT64 pml4_index = MASK_EPT_PML4_INDEX(pfn);
		EPT_ENTRY* pml4_entry = &ept_entry[pml4_index];
		DbgPrint("[*] pml4_entry : %p\n", pml4_entry);
		if (!pml4_entry->AsUInt) {
			EPT_ENTRY* ept_pdpt = (EPT_ENTRY*)EptAllocateEptEntry(page_table);
			if (!ept_pdpt)
				return NULL;

			EptInitTableEntry(pml4_entry, level, VirtualToPhysicalAddress(ept_pdpt));
		}

		EPT_ENTRY* temp = (EPT_ENTRY*)PhysicalToVirtualAddress(pml4_entry->PageFrameNumber << PAGE_SHIFT);
		return EptConstructTables(temp, level - 1, pfn, page_table);
	}

	case 3: {
		// table == PDPT (1 GB)
		const UINT64 pml3_index = MASK_EPT_PML3_INDEX(pfn);
		EPT_ENTRY* pdpt_entry = &ept_entry[pml3_index];
		DbgPrint("[*] pdpt_entry : %p\n", pdpt_entry);
		if (!pdpt_entry->AsUInt) {
			EPT_ENTRY* ept_pde = (EPT_ENTRY*)EptAllocateEptEntry(page_table);
			if (!ept_pde)
				return NULL;

			EptInitTableEntry(pdpt_entry, level, ept_pde->AsUInt);
		}

		EPT_ENTRY* temp = (EPT_ENTRY*)PhysicalToVirtualAddress(pdpt_entry->PageFrameNumber << PAGE_SHIFT);
		return EptConstructTables(temp, level - 1, pfn, page_table);
	}

	case 2: {
		// table == PDT (2 MB)
		const UINT64 pml2_index = MASK_EPT_PML2_INDEX(pfn);
		EPT_ENTRY* pde_entry = &ept_entry[pml2_index];
		DbgPrint("[*] pde_entry : %p\n", pde_entry);
		if (!pde_entry->AsUInt) {
			EPT_ENTRY* ept_pt = EptAllocateEptEntry(page_table);
			if (!ept_pt)
				return NULL;

			EptInitTableEntry(pde_entry, level, ept_pt->AsUInt);
		}

		EPT_ENTRY* temp = (EPT_ENTRY*)PhysicalToVirtualAddress(pde_entry->PageFrameNumber << PAGE_SHIFT);
		return EptConstructTables(temp, level - 1, pfn, page_table);
	}

	case 1: {
		// table == PT (4 KB)
		const UINT64 pte_index = (pfn >> 12) & 0x1ff; //MASK_EPT_PML1_INDEX(pfn);
		EPT_ENTRY* pte_entry = &ept_entry[pte_index];
		NT_ASSERT(!pte_entry->AsUInt);
		EptInitTableEntry(pte_entry, level, pfn);
		return pte_entry;
	}

	default:
		__debugbreak();
		return NULL;
	}
}
