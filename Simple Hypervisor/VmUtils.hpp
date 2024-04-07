#pragma once
#pragma warning(disable : 4201)

#define DRV_NAME		L"\\Device\\Hypervisor"
#define DOS_NAME		L"\\DosDevices\\Hypervisor"
#define VMM_POOL_TAG	'tesT'

#define HOST_STACK_PAGES 6
#define HOST_STACK_SIZE  (HOST_STACK_PAGES * PAGE_SIZE)

using guest_registers = struct guest_registers {
	__m128 xmm[6];
	unsigned __int64 r15;
	unsigned __int64 r14;
	unsigned __int64 r13;
	unsigned __int64 r12;
	unsigned __int64 r11;
	unsigned __int64 r10;
	unsigned __int64 r9;
	unsigned __int64 r8;
	unsigned __int64 rdi;
	unsigned __int64 rsi;
	unsigned __int64 rbp;
	unsigned __int64 rsp;
	unsigned __int64 rbx;
	unsigned __int64 rdx;
	unsigned __int64 rcx;
	unsigned __int64 rax;
};

using vmxon_region_ctx = struct _vmxon_region_ctx {
	union {
		unsigned int all;
		struct {
			unsigned int revision_identifier : 31;
		} bits;
	} header;
	char data[0x1000 - 1 * sizeof(unsigned)];
};

using vmcs_region_ctx = struct _vmcs_region_ctx {
	union {
		unsigned int all;
		struct {
			unsigned int revision_identifier : 31;
			unsigned int shadow_vmcs_indicator : 1;
		} bits;
	} header;

	unsigned int abort_indicator;
	char data[0x1000 - 2 * sizeof(unsigned)];
};

using vcpu_ctx = struct _vcpu_ctx {
	__declspec(align(PAGE_SIZE)) vmxon_region_ctx vmxon;
	__declspec(align(PAGE_SIZE)) vmcs_region_ctx  vmcs;
	__declspec(align(16))		 uint8_t host_stack[HOST_STACK_PAGES];

	uint64_t vmxon_phys;
	uint64_t vmcs_phys;
};

using vmx_ctx = struct _vmx_ctx {
	uint32_t vcpu_count;
	vcpu_ctx vcpus;
};

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
	struct _ept_state* ept_state;
};

typedef struct _ept_error {
	void*	param_1;
	void*	param_2;
} ept_error;

extern unsigned g_num_processors;
extern _vmm_context* vmm_context;
inline vmx_ctx g_vmx_ctx;

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
