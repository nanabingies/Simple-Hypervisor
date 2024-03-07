#pragma once
#pragma warning(disable : 4201)

#define DRV_NAME		L"\\Device\\Hypervisor"
#define DOS_NAME		L"\\DosDevices\\Hypervisor"
#define VMM_POOL_TAG	'tesT'

#define HOST_STACK_SIZE  (20 * PAGE_SIZE)

using guest_registers = struct guest_registers {
	uint64_t	RAX;
	uint64_t	RBX;
	uint64_t	RCX;
	uint64_t	RDX;
	uint64_t	RSP;
	uint64_t	RBP;
	uint64_t	RSI;
	uint64_t	RDI;
	uint64_t	R8;
	uint64_t	R9;
	uint64_t	R10;
	uint64_t	R11;
	uint64_t	R12;
	uint64_t	R13;
	uint64_t	R14;
	uint64_t	R15;
	__m128	xmm[6];
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
