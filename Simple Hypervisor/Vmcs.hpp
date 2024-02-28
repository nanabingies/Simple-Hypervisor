#pragma once
#include "stdafx.h"

#pragma warning(disable : 4115)
#pragma warning(disable : 4201)

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

enum SEGREGS
{
    ES = 0,
    CS,
    SS,
    DS,
    FS,
    GS,
    LDTR,
    TR
};

uint64_t g_GuestRip;
UINT64 g_HostMemory;
UINT64 g_HostRip;

UINT64 g_StackPointerForReturning;
UINT64 g_BasePointerForReturning;

auto inline AsmSetupVmcs(uint32_t) -> EVmErrors;

auto AdjustControls(uint32_t, uint32_t) -> ULONG;

auto SetupVmcs(uint32_t, void*) -> EVmErrors;

auto inline __stdcall AsmHostContinueExecution() -> uint64_t;

auto inline AsmGuestContinueExecution() -> void;

auto inline GetGdtBase() -> uint64_t;
auto inline GetIdtBase() -> uint64_t;

/*USHORT  GetCs(VOID);
USHORT  GetDs(VOID);
USHORT  GetEs(VOID);
USHORT  GetSs(VOID);
USHORT  GetFs(VOID);
USHORT  GetGs(VOID);*/
auto  GetLdtr() -> ushort;
auto  GetTr() -> ushort;
auto  GetIdtLimit() -> ushort;
auto  GetGdtLimit() -> ushort;
auto GetRflags() -> uint64_t;

typedef struct _VMX_GDTENTRY64
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
} VMX_GDTENTRY64, * PVMX_GDTENTRY64;

typedef union _KGDTENTRY64
{
    struct
    {
        uint16_t LimitLow;
        uint16_t BaseLow;
        union
        {
            struct
            {
                uint8_t BaseMiddle;
                uint8_t Flags1;
                uint8_t Flags2;
                uint8_t BaseHigh;
            } Bytes;
            struct
            {
                uint32_t BaseMiddle : 8;
                uint32_t Type : 5;
                uint32_t Dpl : 2;
                uint32_t Present : 1;
                uint32_t LimitHigh : 4;
                uint32_t System : 1;
                uint32_t LongMode : 1;
                uint32_t DefaultBig : 1;
                uint32_t Granularity : 1;
                uint32_t BaseHigh : 8;
            } Bits;
        };
        uint32_t BaseUpper;
        uint32_t MustBeZero;
    };
    struct
    {
        int64_t DataLow;
        int64_t DataHigh;
    };
} KGDTENTRY64, * PKGDTENTRY64;
