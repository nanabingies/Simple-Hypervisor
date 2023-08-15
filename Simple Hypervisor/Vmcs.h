#pragma once
#pragma warning(disable : 4115)

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

typedef union SEGMENT_ATTRIBUTES
{
    USHORT UCHARs;
    struct
    {
        USHORT TYPE : 4; /* 0;  Bit 40-43 */
        USHORT S : 1;    /* 4;  Bit 44 */
        USHORT DPL : 2;  /* 5;  Bit 45-46 */
        USHORT P : 1;    /* 7;  Bit 47 */

        USHORT AVL : 1; /* 8;  Bit 52 */
        USHORT L : 1;   /* 9;  Bit 53 */
        USHORT DB : 1;  /* 10; Bit 54 */
        USHORT G : 1;   /* 11; Bit 55 */
        USHORT GAP : 4;

    } Fields;
} SEGMENT_ATTRIBUTES;

typedef struct SEGMENT_SELECTOR
{
	USHORT             SEL;
	SEGMENT_ATTRIBUTES ATTRIBUTES;
	ULONG32            LIMIT;
	ULONG64            BASE;
} SEGMENT_SELECTOR, * PSEGMENT_SELECTOR;

typedef struct _SEGMENT_DESCRIPTOR
{
    USHORT LIMIT0;
    USHORT BASE0;
    UCHAR  BASE1;
    UCHAR  ATTR0;
    UCHAR  LIMIT1ATTR1;
    UCHAR  BASE2;
} SEGMENT_DESCRIPTOR, * PSEGMENT_DESCRIPTOR;

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


ULONG AdjustControls(ULONG Ctl, ULONG Msr);

EVmErrors SetupVmcs();

UINT64 HostContinueExecution();

UINT64 inline GetGdtBase();
UINT64 inline GetIdtBase();

USHORT  GetCs(VOID);
USHORT  GetDs(VOID);
USHORT  GetEs(VOID);
USHORT  GetSs(VOID);
USHORT  GetFs(VOID);
USHORT  GetGs(VOID);
USHORT  GetLdtr(VOID);
USHORT  GetTr(VOID);
USHORT  GetIdtLimit(VOID);
USHORT  GetGdtLimit(VOID);
ULONG64 GetRflags(VOID);