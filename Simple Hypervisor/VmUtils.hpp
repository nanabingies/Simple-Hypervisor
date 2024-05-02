#pragma once
#pragma warning(disable : 4201)

#define DRV_NAME		L"\\Device\\SimpleHypervisor"
#define DOS_NAME		L"\\DosDevices\\SimpleHypervisor"
#define VMM_POOL_TAG	'tesT'

#define HOST_STACK_SIZE  (20 * PAGE_SIZE)

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

struct __vmcs {
	union {
		unsigned int all;
		struct {
			unsigned int revision_identifier : 31;
			unsigned int shadow_vmcs_indicator : 1;
		};
	} header;
	unsigned int abort_indicator;
	char data[0x1000 - 2 * sizeof(unsigned)];
};

struct __vcpu {
	void* vmm_stack;

	__vmcs* vmcs;
	unsigned __int64 vmcs_physical;

	__vmcs* vmxon;
	unsigned __int64 vmxon_physical;

	struct __vmexit_info {
		//__vmexit_guest_registers* guest_registers;

		unsigned __int64 guest_rip;

		//__rflags guest_rflags;

		unsigned __int64 instruction_length;

		unsigned __int64 reason;

		unsigned __int64 qualification;

		unsigned __int64 instruction_information;

	}vmexit_info;

	struct __vcpu_status {
		unsigned __int64 vmx_on;
		unsigned __int64 vmm_launched;
	}vcpu_status;

	struct __vmx_off_state {
		unsigned __int64  vmx_off_executed;
		unsigned __int64  guest_rip;
		unsigned __int64  guest_rsp;
	}vmx_off_state;

	struct __vcpu_bitmaps {
		unsigned __int8* msr_bitmap;
		unsigned __int64 msr_bitmap_physical;

		unsigned __int8* io_bitmap_a;
		unsigned __int64 io_bitmap_a_physical;

		unsigned __int8* io_bitmap_b;
		unsigned __int64 io_bitmap_b_physical;
	}vcpu_bitmaps;

	//__ept_state* ept_state;
};

struct __vmm_context {
	__vcpu** vcpu_table;
	//pool_manager::__pool_manager* pool_manager;
	//__mtrr_info mtrr_info;
	unsigned __int32 processor_count;
	bool hv_present;
};

extern __vmm_context* g_vmm_context;

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
