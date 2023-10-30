#include "stdafx.h"
#pragma warning(disable : 4996)

static MtrrEntry g_MtrrEntries[numMtrrEntries];

BOOLEAN IsInRange(UINT64 val, UINT64 start, UINT64 end) {
	if (val > start && val <= end)
		return TRUE;

	return FALSE;
}

BOOLEAN CheckEPTSupport() {
	PAGED_CODE();

	IA32_VMX_EPT_VPID_CAP_REGISTER ept_cap;
	ept_cap.AsUInt = __readmsr(IA32_VMX_EPT_VPID_CAP);

	if (!ept_cap.PageWalkLength4 || !ept_cap.MemoryTypeWriteBack || !ept_cap.Invept ||
		!ept_cap.InveptSingleContext || !ept_cap.InveptAllContexts || !ept_cap.Invvpid ||
		!ept_cap.InvvpidIndividualAddress || !ept_cap.InvvpidAllContexts ||
		!ept_cap.InvvpidSingleContext || !ept_cap.InvvpidSingleContextRetainGlobals) {
		return FALSE;
	}

	return TRUE;
}

BOOLEAN EptBuildMTRRMap() {
	PAGED_CODE();

	MtrrEntry* mtrr_entry = g_MtrrEntries;
	RtlSecureZeroMemory(mtrr_entry, numMtrrEntries * sizeof(struct _MtrrEntry));
	
	IA32_MTRR_CAPABILITIES_REGISTER mtrr_cap;
	IA32_MTRR_DEF_TYPE_REGISTER mtrr_def;
	IA32_MTRR_PHYSBASE_REGISTER mtrr_phys_base;
	IA32_MTRR_PHYSMASK_REGISTER mtrr_phys_mask;
	
	mtrr_cap.AsUInt = __readmsr(IA32_MTRR_CAPABILITIES);
	mtrr_def.AsUInt = __readmsr(IA32_MTRR_DEF_TYPE);

	UINT64 varCnt = mtrr_cap.VariableRangeCount;
	UINT64 FixRangeSupport = mtrr_cap.FixedRangeSupported;
	UINT64 FixRangeEnable = mtrr_def.FixedRangeMtrrEnable;
	g_DefaultMemoryType = mtrr_def.DefaultMemoryType;

	if (FixRangeSupport && FixRangeEnable) {
		// Handle Fix Ranged MTRR
		
		static const UINT64 k64kBase = IA32_MTRR_FIX64K_BASE;
		static const UINT64 k64kManagedSize = IA32_MTRR_FIX64K_SIZE;	// 64K
		static const UINT64 k16kBase = IA32_MTRR_FIX16K_BASE;
		static const UINT64 k16kManagedSize = IA32_MTRR_FIX16K_SIZE;
		static const UINT64 k4kBase = IA32_MTRR_FIX4K_BASE;
		static const UINT64 k4kManagedSize = IA32_MTRR_FIX4K_SIZE;

		UINT64 offset = 0x0;
		
		// Let's first set Fix64K MTRR
		Ia32MtrrFixedRangeMsr msr64k;
		msr64k.all = __readmsr(IA32_MTRR_FIX64K_00000);
		for (unsigned idx = 0; idx < 8; idx++) {
			UINT64 base = k64kBase + offset;
			offset += k64kManagedSize;

			// Save the MTRR
			mtrr_entry->MtrrEnabled = TRUE;
			mtrr_entry->MemoryType = msr64k.fields.types[idx];
			mtrr_entry->MtrrFixed = TRUE;
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
				UINT64 base = k16kBase + offset;
				offset += k16kManagedSize;

				// Save the MTRR
				mtrr_entry->MemoryType = msr16k.fields.types[idx];
				mtrr_entry->MtrrEnabled = TRUE;
				mtrr_entry->MtrrFixed = TRUE;
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
				UINT64 base = k4kBase + offset;
				offset += k4kManagedSize;

				// Save the MTRR
				mtrr_entry->MemoryType = msr4k.fields.types[idx];
				mtrr_entry->MtrrEnabled = TRUE;
				mtrr_entry->MtrrFixed = TRUE;
				mtrr_entry->PhysicalAddressStart = base;
				mtrr_entry->PhysicalAddressEnd = base + k4kManagedSize - 1;

				mtrr_entry++;
			}
		}
	}

	for (unsigned iter = 0; iter < varCnt; iter++) {
		mtrr_phys_base.AsUInt = __readmsr(IA32_MTRR_PHYSBASE0 + (iter * 2));
		mtrr_phys_mask.AsUInt = __readmsr(IA32_MTRR_PHYSMASK0 + (iter * 2));

		// The range is invalid
		if (!mtrr_phys_mask.Valid)
			continue;

		// Get the length this MTRR manages
		ULONG length;
		BitScanForward64(&length, mtrr_phys_mask.PageFrameNumber * PAGE_SIZE);

		UINT64 physical_base_start, physical_base_end;
		physical_base_start = mtrr_phys_base.PageFrameNumber * PAGE_SIZE;
		physical_base_end = physical_base_start + ((1ull << length) - 1);

		mtrr_entry->MtrrEnabled = TRUE;
		mtrr_entry->MtrrFixed = FALSE;
		mtrr_entry->MemoryType = mtrr_phys_base.Type;
		mtrr_entry->PhysicalAddressStart = physical_base_start;
		mtrr_entry->PhysicalAddressEnd = physical_base_end;
		gMtrrNum++;
		mtrr_entry++;
	}

	return TRUE;
}

BOOLEAN InitializeEpt(UCHAR processorNumber) {
	PAGED_CODE();

	EptState* ept_state = (EptState*)ExAllocatePoolWithTag(NonPagedPool, sizeof(struct _EptState), VMM_POOL);
	if (!ept_state) {
		DbgPrint("[-] Failed to allocate memory for Ept State.\n");
		return FALSE;
	}
	RtlSecureZeroMemory(ept_state, sizeof(struct _EptState));

	EPT_POINTER* ept_ptr = (EPT_POINTER*)
		(ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, VMM_POOL));
	if (!ept_ptr) {
		DbgPrint("[-] Failed to allocate memory for pointer to Ept.\n");
		return FALSE;
	}
	ept_state->EptPtr = ept_ptr;
	RtlSecureZeroMemory(ept_ptr, PAGE_SIZE);

	
	if (CreateEptState(ept_state) == FALSE) {
		DbgPrint("[-] Failed to set Ept Page Table Entries.\n");
		ExFreePoolWithTag(ept_ptr, VMM_POOL);
		ExFreePoolWithTag(ept_state, VMM_POOL);
		return FALSE;
	}
	
	vmm_context[processorNumber].EptPtr = ept_state->EptPtr->AsUInt;
	vmm_context[processorNumber].EptState = ept_state;

	
	DbgPrint("[*] EPT initialized.\n");
	return TRUE;
}

BOOLEAN CreateEptState(EptState* ept_state) {
	
	EptPageTable* page_table = (EptPageTable*)ExAllocatePoolWithTag(NonPagedPool, sizeof(struct _EptPageTable), VMM_POOL);
	if (!page_table)	return FALSE;
	ept_state->EptPageTable = page_table;

	EPT_PML4E* pml4e = (EPT_PML4E*)&page_table->EptPml4[0];
	EPT_PDPTE* pdpte = (EPT_PDPTE*)&page_table->EptPdpte[0];
	
	ept_state->EptPtr->PageFrameNumber = (VirtualToPhysicalAddress(&pml4e) >> PAGE_SHIFT);
	ept_state->EptPtr->EnableAccessAndDirtyFlags = 0;
	ept_state->EptPtr->MemoryType = WriteBack;
	ept_state->EptPtr->PageWalkLength = MaxEptWalkLength - 1;

	vmm_context[KeGetCurrentProcessorNumber()].EptPml4 = pml4e->AsUInt;

	pml4e->PageFrameNumber = (VirtualToPhysicalAddress(&pdpte) >> PAGE_SHIFT);
	pml4e->ExecuteAccess = 1;
	pml4e->ReadAccess = 1;
	pml4e->UserModeExecute = 1;
	pml4e->WriteAccess = 1;

	EPT_PDPTE pdpte_template = { 0 };
	pdpte_template.ReadAccess = 1;
	pdpte_template.WriteAccess = 1;
	pdpte_template.ExecuteAccess = 1;

	__stosq((SIZE_T*) & page_table->EptPdpte[0], pdpte_template.AsUInt, EPTPDPTEENTRIES);
	for (unsigned idx = 0; idx < EPTPDPTEENTRIES; idx++) {
		page_table->EptPdpte[idx].PageFrameNumber = (VirtualToPhysicalAddress(&page_table->EptPde[idx][0]) >> PAGE_SHIFT);
	}

	EPT_PDE_2MB pde_template = { 0 };
	pde_template.ReadAccess = 1;
	pde_template.WriteAccess = 1;
	pde_template.ExecuteAccess = 1;
	pde_template.LargePage = 1;
	
	__stosq((SIZE_T*) & page_table->EptPde[0], pde_template.AsUInt, EPTPDEENTRIES);
	for (unsigned i = 0; i < EPTPML4ENTRIES; i++) {
		for (unsigned j = 0; j < EPTPDPTEENTRIES; j++) {
			SetupPml2Entries(ept_state, &page_table->EptPde[i][j], (i * 512) + j);
		}
	}

	ept_state->GuestAddressWidthValue = MaxEptWalkLength - 1;
	return TRUE;
}

VOID SetupPml2Entries(EptState* ept_state, EPT_PDE_2MB* pde_entry, UINT64 pfn) {
	UNREFERENCED_PARAMETER(ept_state);

	pde_entry->PageFrameNumber = pfn;

	UINT64 addrOfPage = pfn * PAGE2MB;
	if (pfn == 0) {
		pde_entry->MemoryType = Uncacheable;
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

	pde_entry->MemoryType = memory_type;

	return;
}

BOOLEAN IsValidForLargePage(UINT64 pfn) {
	UNREFERENCED_PARAMETER(pfn);

	UINT64 page_start = pfn * PAGE2MB;
	UINT64 page_end = (pfn * PAGE2MB) + (PAGE2MB - 1);

	MtrrEntry* temp = (MtrrEntry*)g_MtrrEntries;

	for (unsigned idx = 0; idx < gMtrrNum; idx++) {
		if (page_start <= temp[idx].PhysicalAddressEnd && page_end > temp[idx].PhysicalAddressEnd)
			return FALSE;

		else if (page_start < temp[idx].PhysicalAddressStart && page_end >= temp[idx].PhysicalAddressStart)
			return FALSE;
	}

	return TRUE;
}

UINT64 GetMemoryType(UINT64 pfn, BOOLEAN large_page) {
	UINT64 page_start = large_page == TRUE ? pfn * PAGE2MB : pfn * PAGE_SIZE;
	UINT64 page_end = large_page == TRUE ? (pfn * PAGE2MB) + (PAGE2MB - 1) : (pfn * PAGE_SIZE) + (PAGE_SIZE - 1);
	UINT64 memory_type = g_DefaultMemoryType;

	MtrrEntry* temp = (MtrrEntry*)g_MtrrEntries;

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

EPT_PDE_2MB* GetPdeEntry(EptPageTable* page_table, UINT64 pfn) {
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
	InsertHeadList(&page_table->DynamicPages, &split_page->SplitPages);

	return;
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
	UINT64 pml4e = vmm_context[KeGetCurrentProcessorNumber()].EptPml4;
	EptpConstructTables(pml4e, 4, phys_addr, ept_state->EptPageTable);

	// invalidate Global EPT entries
	EptInvGlobalEntry();

	return;
}

EPT_ENTRY* EptpConstructTables(UINT64 pml4e, UINT64 level, UINT64 phys_addr, EptPageTable* page_table) {

}
