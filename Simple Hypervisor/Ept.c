#include "stdafx.h"

void InitializeEpt() {
	EPT_POINTER* EptPtr = (EPT_POINTER*)
		(ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, VMM_POOL));
	if (!EptPtr) {
		DbgPrint("[-] Failed to allocate memory for pointer to Ept.\n");
		return;
	}

	RtlSecureZeroMemory(EptPtr, PAGE_SIZE);
	DbgPrint("[*] Pointer to EPT at : %llx\n", (UINT64)EptPtr);

	vmm_context->eptPtr = (UINT64)EptPtr;

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

	g_GuestMemory = (UINT64)guest_memory;
	RtlSecureZeroMemory(guest_memory, numPagesToAllocate * PAGE_SIZE);

	for (size_t i = 0; i < (100 * PAGE_SIZE) - 1; i++) {
		void* TempAsm = "\xF4";     // HLT asm opcode
		memcpy((void*)(g_GuestMemory + i), TempAsm, 1);
	}

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
	pde->PageFrameNumber = (VirtualToPhysicalAddress(pte) / PAGE_SIZE);
	pde->ReadAccess = 1;
	pde->UserModeExecute = 1;
	pde->WriteAccess = 1;

	// 
	// Update PDPTE
	//
	//pdpte->Accessed = 1;
	pdpte->ExecuteAccess = 1;
	pdpte->PageFrameNumber = (VirtualToPhysicalAddress(pde) / PAGE_SIZE);
	pdpte->ReadAccess = 1;
	pdpte->UserModeExecute = 1;
	pdpte->WriteAccess = 1;

	//
	// Update PML4E
	//
	//pml4e->Accessed = 1;
	pml4e->ExecuteAccess = 1;
	pml4e->PageFrameNumber = (VirtualToPhysicalAddress(pdpte) / PAGE_SIZE);
	pml4e->ReadAccess = 1;
	pml4e->UserModeExecute = 1;
	pml4e->WriteAccess = 1;

	//
	// Update EPT Ptr
	//
	EptPtr->EnableAccessAndDirtyFlags = 1;
	EptPtr->EnableSupervisorShadowStackPages = 1;
	EptPtr->MemoryType = 6; // WriteBack
	EptPtr->PageFrameNumber = (VirtualToPhysicalAddress(pml4e) / PAGE_SIZE);
	EptPtr->PageWalkLength = 3;

	DbgPrint("[*] EPT Setup Done.\n");

	return;
}