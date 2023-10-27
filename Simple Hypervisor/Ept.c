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
	//INT index = 0;
	
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
		mtrr_entry->MemoryType = mtrr_phys_base.Type;
		mtrr_entry->PhysicalAddressStart = physical_base_start;
		mtrr_entry->PhysicalAddressEnd = physical_base_end;
		mtrr_entry++;
	}

	
	/*MtrrEntry* temp = (MtrrEntry*)g_MtrrEntries;
	DbgPrint("[*][Debugging] temp : %p\n", (PVOID)temp);
	do {
		DbgPrint("[*][Debugging] MtrrEnabled : %p\n", (PVOID)temp->MtrrEnabled);
		DbgPrint("[*][Debugging] MemoryType : %p\n", (PVOID)temp->MemoryType);
		DbgPrint("[*][Debugging] MtrrFixed : %p\n", (PVOID)temp->MtrrFixed);
		DbgPrint("[*][Debugging] PhysicalAddressStart : %p\n", (PVOID)temp->PhysicalAddressStart);
		DbgPrint("[*][Debugging] PhysicalAddressEnd : %p\n", (PVOID)temp->PhysicalAddressEnd);

		temp = (MtrrEntry*)((UCHAR*)temp + sizeof(struct _MtrrEntry));
	} while (temp->PhysicalAddressEnd != 0x0);*/

	return TRUE;
}

/*BOOLEAN InitializeEpt(UCHAR processorNumber) {
	PAGED_CODE();

	EPT_POINTER* EptPtr = (EPT_POINTER*)
		(ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, VMM_POOL));
	if (!EptPtr) {
		DbgPrint("[-] Failed to allocate memory for pointer to Ept.\n");
		return FALSE;
	}

	RtlSecureZeroMemory(EptPtr, PAGE_SIZE);
	DbgPrint("[*] Pointer to EPT at : %llx\n", (UINT64)EptPtr);


	EPT_PML4E* pml4e = (EPT_PML4E*)
		(ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, VMM_POOL));
	if (!pml4e) {
		DbgPrint("[-] Failed to allocate memory for pml4e.\n");
		ExFreePoolWithTag(EptPtr, VMM_POOL);

		return FALSE;
	}

	RtlSecureZeroMemory(pml4e, PAGE_SIZE);
	DbgPrint("[*] pml4e is at : %llx\n", (UINT64)pml4e);

	EPT_PDPTE* pdpte = (EPT_PDPTE*)
		(ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, VMM_POOL));
	if (!pdpte) {
		DbgPrint("[-] Failed to allocate memory for pdpte.\n");

		ExFreePoolWithTag(EptPtr, VMM_POOL);
		ExFreePoolWithTag(pml4e, VMM_POOL);
		return FALSE;
	}

	RtlSecureZeroMemory(pdpte, PAGE_SIZE);
	DbgPrint("[*] pdpte is at : %llx\n", (UINT64)pdpte);

	EPT_PDE* pde = (EPT_PDE*)
		(ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, VMM_POOL));
	if (!pde) {
		DbgPrint("[-] Failed to allocate memory for pde.\n");

		ExFreePoolWithTag(EptPtr, VMM_POOL);
		ExFreePoolWithTag(pml4e, VMM_POOL);
		ExFreePoolWithTag(pdpte, VMM_POOL);
		return FALSE;
	}

	RtlSecureZeroMemory(pde, PAGE_SIZE);
	DbgPrint("[*] pde is at : %llx\n", (UINT64)pde);

	EPT_PTE* pte = (EPT_PTE*)
		(ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, VMM_POOL));
	if (!pte) {
		DbgPrint("[-] Failed to allocate memory for pte.\n");

		ExFreePoolWithTag(EptPtr, VMM_POOL);
		ExFreePoolWithTag(pml4e, VMM_POOL);
		ExFreePoolWithTag(pdpte, VMM_POOL);
		ExFreePoolWithTag(pde, VMM_POOL);
		return FALSE;
	}

	RtlSecureZeroMemory(pte, PAGE_SIZE);
	DbgPrint("[*] pte is at : %llx\n", (UINT64)pte);


	// 
	// One page for RIP and another one page for RSP
	// Physical Address of the Page should be easily divisible by 4096 (0x1000) 
	// since the last 12 bits will be used in chosing the PAGE to use
	// https://rayanfam.com/topics/hypervisor-from-scratch-part-4/ 
	//
	
	EPT_ENTRY* guest_memory = (EPT_ENTRY*)
		(ExAllocatePoolWithTag(NonPagedPool, numPagesToAllocate * PAGE_SIZE, VMM_POOL));
	if (!guest_memory)	return FALSE;
	DbgPrint("[*] Guest memory allocated at %llx\n", (UINT64)guest_memory);

	g_GuestMemory = (UINT64)guest_memory;
	RtlSecureZeroMemory(guest_memory, numPagesToAllocate * PAGE_SIZE);

	//
	// Update PTE 
	//
	for (auto idx = 0; idx < numPagesToAllocate; ++idx) {
		//pte[idx].Accessed = 1;
		//pte[idx].Dirty = 1;
		pte[idx].ExecuteAccess = 1;
		pte[idx].MemoryType = WriteBack;
		pte[idx].PageFrameNumber = (VirtualToPhysicalAddress(guest_memory + (idx * PAGE_SIZE)) / PAGE_SIZE);
		pte[idx].PagingWriteAccess = 1;
		pte[idx].ReadAccess = 1;
		pte[idx].SubPageWritePermissions = 1;
		pte[idx].UserModeExecute = 1;
		pte[idx].WriteAccess = 1;
	}


	//
	// Update PDE
	//
	//pde->Accessed = 1;
	pde->ExecuteAccess = 1;
	pde->PageFrameNumber = (VirtualToPhysicalAddress(pte) >> PAGE_SHIFT);		// / PAGE_SIZE
	pde->ReadAccess = 1;
	pde->UserModeExecute = 1;
	pde->WriteAccess = 1;

	// 
	// Update PDPTE
	//
	//pdpte->Accessed = 1;
	pdpte->ExecuteAccess = 1;
	pdpte->PageFrameNumber = (VirtualToPhysicalAddress(pde) >> PAGE_SHIFT);	// / PAGE_SIZE
	pdpte->ReadAccess = 1;
	pdpte->UserModeExecute = 1;
	pdpte->WriteAccess = 1;

	//
	// Update PML4E
	//
	//pml4e->Accessed = 1;
	pml4e->ExecuteAccess = 1;
	pml4e->PageFrameNumber = (VirtualToPhysicalAddress(pdpte) >> PAGE_SHIFT);	// / PAGE_SIZE
	pml4e->ReadAccess = 1;
	pml4e->UserModeExecute = 1;
	pml4e->WriteAccess = 1;

	//
	// Update EPT Ptr
	//
	EptPtr->EnableAccessAndDirtyFlags = 0;	//
	//EptPtr->EnableSupervisorShadowStackPages = 1;
	EptPtr->MemoryType = WriteBack;
	EptPtr->PageFrameNumber = (VirtualToPhysicalAddress(pml4e) >> PAGE_SHIFT);	// / PAGE_SIZE
	EptPtr->PageWalkLength = MaxEptWalkLength - 1;

	vmm_context[processorNumber].EptPtr = EptPtr->AsUInt;
	vmm_context[processorNumber].EptPml4 = pml4e->AsUInt;

	return TRUE;
}*/

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

	EPT_PML4E* pml4e = (EPT_PML4E*)&page_table->EptPml4[0];
	EPT_PDPTE* pdpte = (EPT_PDPTE*)&page_table->EptPdpte[0];
	EPT_PDE_2MB* pde = (EPT_PDE_2MB*)&page_table->EptPde[0];

	ept_state->EptPtr->PageFrameNumber = (VirtualToPhysicalAddress(pml4e) >> PAGE_SHIFT);
	ept_state->EptPtr->EnableAccessAndDirtyFlags = 0;
	ept_state->EptPtr->MemoryType = WriteBack;
	ept_state->EptPtr->PageWalkLength = MaxEptWalkLength - 1;

	pml4e->PageFrameNumber = (VirtualToPhysicalAddress(pdpte) >> PAGE_SHIFT);
	pml4e->ExecuteAccess = 1;
	pml4e->ReadAccess = 1;
	pml4e->UserModeExecute = 1;
	pml4e->WriteAccess = 1;

	EPT_PDPTE pdpte_template = { 0 };
	pdpte_template.ReadAccess = 1;
	pdpte_template.WriteAccess = 1;
	pdpte_template.ExecuteAccess = 1;

	__stosq(&pdpte->AsUInt, pdpte_template.AsUInt, EPTPDPTEENTRIES);
	for (unsigned idx = 0; idx < EPTPDPTEENTRIES; idx++) {
		//pdpte->PageFrameNumber = (VirtualToPhysicalAddress(pde) >> PAGE_SHIFT);
		page_table->EptPml4[idx].PageFrameNumber = (VirtualToPhysicalAddress(page_table->EptPde[idx]) >> PAGE_SHIFT);
	}

	EPT_PDE_2MB pde_template = { 0 };
	pde_template.ReadAccess = 1;
	pde_template.WriteAccess = 1;
	pde_template.ExecuteAccess = 1;
	pde_template.LargePage = 1;
	
	__stosq(&pde->AsUInt, pde_template.AsUInt, EPTPDEENTRIES);
	for (unsigned i = 0; i < EPTPML4ENTRIES; i++) {
		for (unsigned j = 0; j < EPTPDPTEENTRIES; j++) {
			SetupPml2Entries(ept_state, page_table->EptPde[i][j], (i * 512) + j);
		}
	}

	ept_state->EptPageTable = page_table;
	ept_state->GuestAddressWidthValue = MaxEptWalkLength - 1;
	return TRUE;
}

VOID SetupPml2Entries(EptState* ept_state, EPT_PDE_2MB pde_entry, UINT64 pfn) {
	UNREFERENCED_PARAMETER(ept_state);
	
	pde_entry.PageFrameNumber = pfn;

	/*if (pfn == 0) {
		pde_entry.MemoryType = Uncacheable;
		return;
	}

	UINT64 memory_type = WriteBack;
	UINT64 AddressOfPage = pfn * PAGE2MB;

	// loop MTRR and set memory types
	MtrrEntry* temp = (MtrrEntry*)g_MtrrEntries;
	do {
		 
		if (IsInRange(AddressOfPage, temp->PhysicalAddressStart, temp->PhysicalAddressEnd)) {
			memory_type = temp->MemoryType;
			if (memory_type == Uncacheable) {
				// If this is going to be marked uncacheable, then we stop the search as UC always takes precedent.
				break;
			}
		}

		temp = (MtrrEntry*)((UCHAR*)temp + sizeof(struct _MtrrEntry));
	} while (temp->PhysicalAddressEnd != 0x0);

	pde_entry.MemoryType = memory_type;*/

	if (IsValidForLargePage(pfn)) {
		pde_entry.MemoryType = GetMemoryType(pfn, TRUE);
		return TRUE;
	}
	else {

	}

	return;
}

BOOLEAN IsValidForLargePage(UINT64 pfn) {

}

UINT64 GetMemoryType(UINT64 pfn, BOOLEAN large_page) {

}
