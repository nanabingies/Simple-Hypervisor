#pragma once
#pragma warning(disable : 4201)
#pragma warning(disable : 4408)
#pragma warning(disable : 4115)

#define RPL_MASK                3
#define SELECTOR_TABLE_INDEX    0x04

#define MSR_IA32_SYSENTER_CS	0x174
#define MSR_IA32_SYSENTER_ESP	0x175
#define MSR_IA32_SYSENTER_EIP	0x176
#define MSR_IA32_DEBUGCTL		0x1D9

#define VMCS_GUEST_DEBUGCTL_HIGH	0x2803

using EVmErrors = enum EVmErrors {
	VM_ERROR_OK = 0,
	VM_ERROR_ERR_INFO_OK,
	VM_ERROR_ERR_INFO_ERR,
};

enum SEGREGS {
    ES = 0,
    CS,
    SS,
    DS,
    FS,
    GS,
    LDTR,
    TR
};

typedef struct _vmx_gdtentry64
{
    uint64_t Base;
    uint32_t Limit;
    union
    {
        struct
        {
            uint8_t Flags1;
            uint8_t Flags2;
            uint8_t Flags3;
            uint8_t Flags4;
        } Bytes;
        struct
        {
            uint16_t SegmentType : 4;
            uint16_t DescriptorType : 1;
            uint16_t Dpl : 2;
            uint16_t Present : 1;

            uint16_t Reserved : 4;
            uint16_t System : 1;
            uint16_t LongMode : 1;
            uint16_t DefaultBig : 1;
            uint16_t Granularity : 1;

            uint16_t Unusable : 1;
            uint16_t Reserved2 : 15;
        } Bits;
        uint32_t AccessRights;
    };
    uint16_t Selector;
} vmx_gdtentry64, *pvmx_gdtentry64;

typedef union _KGDTENTRY64
{
    struct
    {
        UINT16 LimitLow;
        UINT16 BaseLow;
        union
        {
            struct
            {
                UINT8 BaseMiddle;
                UINT8 Flags1;
                UINT8 Flags2;
                UINT8 BaseHigh;
            } Bytes;
            struct
            {
                UINT32 BaseMiddle : 8;
                UINT32 Type : 5;
                UINT32 Dpl : 2;
                UINT32 Present : 1;
                UINT32 LimitHigh : 4;
                UINT32 System : 1;
                UINT32 LongMode : 1;
                UINT32 DefaultBig : 1;
                UINT32 Granularity : 1;
                UINT32 BaseHigh : 8;
            } Bits;
        };
        UINT32 BaseUpper;
        UINT32 MustBeZero;
    };
    struct
    {
        INT64 DataLow;
        INT64 DataHigh;
    };
} KGDTENTRY64, * PKGDTENTRY64;

extern "C" {
    unsigned long inline ret_val;
    unsigned __int64 inline cr3_val;

    auto inline asm_setup_vmcs(unsigned long) -> EVmErrors;

    auto inline asm_guest_continue_execution() -> void;

    auto inline asm_get_ldtr() -> unsigned short;
    auto inline asm_get_tr() -> unsigned short;
    auto inline asm_get_idt_limit() -> unsigned short;
    auto inline asm_get_gdt_limit() -> unsigned short;
    auto inline asm_get_rflags() -> unsigned __int64;

    auto inline asm_get_gdt_base() -> unsigned __int64;
    auto inline asm_get_idt_base() -> unsigned __int64;
}

auto setup_vmcs(unsigned long, void*, uint64_t) -> EVmErrors;

auto hv_setup_vmcs(struct __vcpu*, void*) -> void;

extern "C" auto inline asm_restore_vmm_state() -> void;

extern "C" auto inline asm_host_continue_execution() -> void;
