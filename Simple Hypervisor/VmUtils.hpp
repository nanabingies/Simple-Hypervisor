#pragma once
#pragma warning(disable : 4201)

#define DRV_NAME		L"\\Device\\Hypervisor"
#define DOS_NAME		L"\\DosDevices\\Hypervisor"
#define VMM_POOL_TAG	'tesT'

#define HOST_STACK_PAGES 6
#define HOST_STACK_SIZE  (HOST_STACK_PAGES * PAGE_SIZE)

#define MAX_CORE_COUNT	64

union cr_fixed_t {
	struct {
		unsigned long low;
		long high;
	} split;
	struct {
		unsigned long low;
		long high;
	} u;
	long long all;
};

using guest_registers = struct guest_registers {
	__m128 xmm0;
	__m128 xmm1;
	__m128 xmm2;
	__m128 xmm3;
	__m128 xmm4;
	__m128 xmm5;
	__m128 xmm6;
	__m128 xmm7;
	__m128 xmm8;
	__m128 xmm9;
	__m128 xmm10;
	__m128 xmm11;
	__m128 xmm12;
	__m128 xmm13;
	__m128 xmm14;
	__m128 xmm15;

	uint64_t padding;

	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t r11;
	uint64_t r10;
	uint64_t r9;
	uint64_t r8;
	uint64_t rbp;
	uint64_t rdi;
	uint64_t rsi;
	uint64_t rdx;
	uint64_t rcx;
	uint64_t rbx;
	uint64_t rax;
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

typedef struct _vcpu_ctx {
	__declspec(align(PAGE_SIZE)) vmxon_region_ctx vmxon;
	__declspec(align(PAGE_SIZE)) vmcs_region_ctx  vmcs;
	__declspec(align(PAGE_SIZE)) vmcs_region_ctx  io_bitmap_a;
	__declspec(align(PAGE_SIZE)) vmcs_region_ctx  io_bitmap_b;
	__declspec(align(PAGE_SIZE)) vmcs_region_ctx  msr_bitmap;
	__declspec(align(16))		 uint8_t host_stack[HOST_STACK_PAGES];

	uint64_t vmxon_phys;
	uint64_t vmcs_phys;
	uint64_t io_bitmap_a_phys;
	uint64_t io_bitmap_b_phys;
	uint64_t msr_bitmap_phys;
}vcpu_ctx, * pvcpu_ctx;

typedef struct _vmx_ctx {
	uint32_t vcpu_count;
	vcpu_ctx vcpus[MAX_CORE_COUNT];
}vmx_ctx, * pvmx_ctx;

inline struct _vmx_ctx g_vmx_ctx;

typedef struct _ept_error {
	void*	param_1;
	void*	param_2;
} ept_error;

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
