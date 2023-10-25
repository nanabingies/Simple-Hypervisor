#include "stdafx.h"
#pragma warning(disable : 4996)

static struct MtrrEntry g_MtrrEntries[numMtrrEntries];

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

	struct MtrrEntry* mtrr_entry = g_MtrrEntries;
	RtlSecureZeroMemory(mtrr_entry, numMtrrEntries * sizeof(struct MtrrEntry));
	
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
		DbgPrint("[*] Fixed Range MTRR supported.\n");
		DbgPrint("[*] Add support later.\n");

		//
		//	TODO: Add Fixed Range MTRR
		//
		static const UINT64 k64kBase = IA32_MTRR_FIX64K_BASE;
		static const UINT64 k64kManagedSize = IA32_MTRR_FIX64K_SIZE;	// 64K
		static const UINT64 k16kBase = IA32_MTRR_FIX16K_BASE;
		static const UINT64 k16kManagedSize = IA32_MTRR_FIX16K_SIZE;
		static const UINT64 k4kBase = IA32_MTRR_FIX4K_BASE;
		static const UINT64 k4kManagedSize = IA32_MTRR_FIX4K_SIZE;
		
		// Let's first set 64K page data
		__readmsr(IA32_MTRR_FIX64K_00000);

		// let's set 16k page data
		__readmsr(IA32_MTRR_FIX16K_80000);

		// let's set 4k page data
		__readmsr(IA32_MTRR_FIX4K_C0000);
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

		mtrr_entry->MtrrEnabled = 0x1;
		mtrr_entry->MemoryType = mtrr_phys_base.Type;
		mtrr_entry->PhysicalAddressStart = physical_base_start;
		mtrr_entry->PhysicalAddressEnd = physical_base_end;
		mtrr_entry++;
	}

	
	/*struct MtrrEntry* temp = (struct MtrrEntry*)g_MtrrEntries;
	do {
		
		temp = (struct MtrrEntry*)((UCHAR*)temp + sizeof(struct MtrrEntry));
	} while (temp->PhysicalAddressEnd != 0x0);*/

	return TRUE;
}

void InitializeEpt(UCHAR processorNumber) {
	PAGED_CODE();

	EPT_POINTER* EptPtr = (EPT_POINTER*)
		(ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, VMM_POOL));
	if (!EptPtr) {
		DbgPrint("[-] Failed to allocate memory for pointer to Ept.\n");
		return;
	}

	RtlSecureZeroMemory(EptPtr, PAGE_SIZE);
	DbgPrint("[*] Pointer to EPT at : %llx\n", (UINT64)EptPtr);


	EPT_PML4E* pml4e = (EPT_PML4E*)
		(ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, VMM_POOL));
	if (!pml4e) {
		DbgPrint("[-] Failed to allocate memory for pml4e.\n");
		ExFreePoolWithTag(EptPtr, VMM_POOL);

		return;
	}

	RtlSecureZeroMemory(pml4e, PAGE_SIZE);
	DbgPrint("[*] pml4e is at : %llx\n", (UINT64)pml4e);

	EPT_PDPTE* pdpte = (EPT_PDPTE*)
		(ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, VMM_POOL));
	if (!pdpte) {
		DbgPrint("[-] Failed to allocate memory for pdpte.\n");

		ExFreePoolWithTag(EptPtr, VMM_POOL);
		ExFreePoolWithTag(pml4e, VMM_POOL);
		return;
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
		return;
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
		return;
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
	if (!guest_memory)	return;
	DbgPrint("[*] Guest memory allocated at %llx\n", (UINT64)guest_memory);

	g_GuestMemory = (UINT64)guest_memory;
	RtlSecureZeroMemory(guest_memory, numPagesToAllocate * PAGE_SIZE);

	//
	// Update PTE 
	//
	for (auto idx = 0; idx < numPagesToAllocate; ++idx) {
		//pte[idx].Accessed = 1;
		pte[idx].Dirty = 1;
		pte[idx].ExecuteAccess = 1;
		pte[idx].MemoryType = 0x6; // WriteBack
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
	EptPtr->EnableAccessAndDirtyFlags = 1;
	EptPtr->EnableSupervisorShadowStackPages = 1;
	EptPtr->MemoryType = g_DefaultMemoryType;	// WriteBack
	EptPtr->PageFrameNumber = (VirtualToPhysicalAddress(pml4e) >> PAGE_SHIFT);	// / PAGE_SIZE
	EptPtr->PageWalkLength = MaxEptWalkLength - 1;

	vmm_context[processorNumber].eptPtr = EptPtr->AsUInt;
	vmm_context[processorNumber].pml4e = pml4e->AsUInt;

	DbgPrint("[*] EPT Setup Done.\n");

	return;
}