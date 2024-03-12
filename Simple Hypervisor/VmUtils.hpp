#pragma once
#pragma warning(disable : 4201)

#define DRV_NAME		L"\\Device\\Hypervisor"
#define DOS_NAME		L"\\DosDevices\\Hypervisor"
#define VMM_POOL_TAG	'tesT'

#define HOST_STACK_SIZE  (20 * PAGE_SIZE)

using guest_registers = struct guest_registers {
	uint64_t	rax;
	uint64_t	rbx;
	uint64_t	rcx;
	uint64_t	rdx;
	uint64_t	rsp;
	uint64_t	rbp;
	uint64_t	rsi;
	uint64_t	rdi;
	uint64_t	r8;
	uint64_t	r9;
	uint64_t	r10;
	uint64_t	r11;
	uint64_t	r12;
	uint64_t	r13;
	uint64_t	r14;
	uint64_t	r15;
	__m128		xmm[6];
} ;

struct _vmm_context {
	uint64_t	vmxon_region_virt_addr;				// Virtual address of VMXON Region
	uint64_t	vmxon_region_phys_addr;				// Physical address of VMXON Region

	uint64_t	vmcs_region_virt_addr;				// virtual address of VMCS Region
	uint64_t	vmcs_region_phys_addr;				// Physical address of VMCS Region

	uint64_t	io_bitmap_a_virt_addr;              // Virtual address of I/O bitmap A
	uint64_t	io_bitmap_a_phys_addr;              // Physical address

	uint64_t	io_bitmap_b_virt_addr;				// Virtual address of I/O bitmap B
	uint64_t	io_bitmap_b_phys_addr;				// Plysical address

	uint64_t	msr_bitmap_virt_addr;				// Virtual address of MSR bitmap
	uint64_t	msr_bitmap_phys_addr;				// Physical address

	uint64_t	host_stack;							// Stack of the VM Exit Handler

	uint64_t	guest_memory;						// Guest RSP

	uint64_t	guest_rsp;
	uint64_t	guest_rip;

	uint64_t	host_rsp;
	uint64_t	host_rip;

	struct {
		uint64_t	g_stack_pointer_for_returning;
		uint64_t	g_base_pointer_for_returning;
	};

	struct {
		// vm exit info
		guest_registers* guest_regs;

		uint64_t vmexit_guest_rip;
		uint64_t vmexit_guest_rsp;

		uint64_t vmexit_reason;
		uint64_t vmexit_qualification;

		uint64_t vmexit_instruction_information;
		uint64_t vmexit_instruction_length;
	};

	uint64_t	ept_ptr;
	uint64_t	ept_pml4;
	//ept_state* ept_state;
};

extern bool vm_off;
extern unsigned g_num_processors;
extern _vmm_context* vmm_context;

inline uint64_t physical_to_virtual_address(uint64_t physical_address) {
	PHYSICAL_ADDRESS physAddr;
	physAddr.QuadPart = physical_address;

	return reinterpret_cast<uint64_t>(MmGetVirtualForPhysical(physAddr));
}

inline uint64_t virtual_to_physical_address(void* virtual_address) {
	PHYSICAL_ADDRESS physAddr;

	physAddr = MmGetPhysicalAddress(virtual_address);
	return physAddr.QuadPart;
}

extern "C" {
	NTKERNELAPI
		_IRQL_requires_max_(APC_LEVEL)
		_IRQL_requires_min_(PASSIVE_LEVEL) _IRQL_requires_same_ VOID
		KeGenericCallDpc(_In_ PKDEFERRED_ROUTINE Routine, _In_opt_ PVOID Context);

	NTKERNELAPI
		_IRQL_requires_(DISPATCH_LEVEL) _IRQL_requires_same_ VOID
		KeSignalCallDpcDone(_In_ PVOID SystemArgument1);

	NTKERNELAPI
		_IRQL_requires_(DISPATCH_LEVEL) _IRQL_requires_same_ LOGICAL
		KeSignalCallDpcSynchronize(_In_ PVOID SystemArgument2);
}
