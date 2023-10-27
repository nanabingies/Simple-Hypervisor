#pragma once
#pragma warning(disable : 4115)
#pragma warning(disable : 4201)

#define RPL_MASK                3
#define SELECTOR_TABLE_INDEX    0x04

#define MSR_IA32_SYSENTER_CS	0x174
#define MSR_IA32_SYSENTER_ESP	0x175
#define MSR_IA32_SYSENTER_EIP	0x176
#define MSR_IA32_DEBUGCTL		0x1D9

#define VMCS_GUEST_DEBUGCTL_HIGH	0x2803

typedef enum EVmErrors
{
	VM_ERROR_OK = 0,
	VM_ERROR_ERR_INFO_OK,
	VM_ERROR_ERR_INFO_ERR,
}EVmErrors;

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

UINT64 g_GuestRip;
UINT64 g_HostMemory;
UINT64 g_HostRip;

UINT64 g_StackPointerForReturning;
UINT64 g_BasePointerForReturning;

EVmErrors inline AsmSetupVmcs(ULONG);

ULONG AdjustControls(ULONG Ctl, ULONG Msr);

EVmErrors SetupVmcs(ULONG, PVOID);

inline UINT64 __stdcall AsmHostContinueExecution();

inline VOID AsmGuestContinueExecution();

UINT64 inline GetGdtBase();
UINT64 inline GetGdtBase();
UINT64 inline GetIdtBase();

/*USHORT  GetCs(VOID);
USHORT  GetDs(VOID);
USHORT  GetEs(VOID);
USHORT  GetSs(VOID);
USHORT  GetFs(VOID);
USHORT  GetGs(VOID);*/
USHORT  GetLdtr(VOID);
USHORT  GetTr(VOID);
USHORT  GetIdtLimit(VOID);
USHORT  GetGdtLimit(VOID);
ULONG64 GetRflags(VOID);

typedef struct _VMX_GDTENTRY64
{
    UINT64 Base;
    UINT32 Limit;
    union
    {
        struct
        {
            UINT8 Flags1;
            UINT8 Flags2;
            UINT8 Flags3;
            UINT8 Flags4;
        } Bytes;
        struct
        {
            UINT16 SegmentType : 4;
            UINT16 DescriptorType : 1;
            UINT16 Dpl : 2;
            UINT16 Present : 1;

            UINT16 Reserved : 4;
            UINT16 System : 1;
            UINT16 LongMode : 1;
            UINT16 DefaultBig : 1;
            UINT16 Granularity : 1;

            UINT16 Unusable : 1;
            UINT16 Reserved2 : 15;
        } Bits;
        UINT32 AccessRights;
    };
    UINT16 Selector;
} VMX_GDTENTRY64, * PVMX_GDTENTRY64;

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
