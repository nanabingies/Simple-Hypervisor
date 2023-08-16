#pragma once
#pragma warning(disable : 4201)

#define DRV_NAME	L"\\Device\\Hypervisor"
#define DOS_NAME	L"\\DosDevices\\Hypervisor"
#define VMM_POOL	'tesT'

#define STACK_SIZE  (8 * PAGE_SIZE)
BOOLEAN VmOff;

struct _vmm_context {
	UINT64	vmxonRegionVirt;				// Virtual address of VMXON Region
	UINT64	vmxonRegionPhys;				// Physical address of VMXON Region

	UINT64	vmcsRegionVirt;					// virtual address of VMCS Region
	UINT64	vmcsRegionPhys;					// Physical address of VMCS Region

	UINT64    bitmapVirt;                     // Virtual address of bitmap bits
	UINT64    bitmapPhys;                     // Physical address

	UINT64	eptPtr;							// Pointer to the EPT

	UINT64	GuestStack;						// Stack of the VM Exit Handler
};


struct driverGlobals {
	PDEVICE_OBJECT	g_DeviceObject;
	LIST_ENTRY		g_ListofDevices;
};

struct _vmm_context* vmm_context;
struct driverGlobals* g_DriverGlobals;
UINT64 g_GuestMemory;

ULONG64 PhysicalToVirtualAddress(UINT64 physical_address);
UINT64 VirtualToPhysicalAddress(void* virtual_address);


struct _VMX_GDTENTRY64
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
}; /*VMX_GDTENTRY64, * PVMX_GDTENTRY64;*/

//0x10 bytes (sizeof)
typedef union _kgdtentry64
{
    struct
    {
        USHORT LimitLow;                                                    //0x0
        USHORT BaseLow;                                                     //0x2
    };
    struct
    {
        UCHAR BaseMiddle;                                                   //0x4
        UCHAR Flags1;                                                       //0x5
        UCHAR Flags2;                                                       //0x6
        UCHAR BaseHigh;                                                     //0x7
    } Bytes;                                                                //0x4
    struct
    {
        struct
        {
            ULONG BaseMiddle : 8;                                                 //0x4
            ULONG Type : 5;                                                       //0x4
            ULONG Dpl : 2;                                                        //0x4
            ULONG Present : 1;                                                    //0x4
            ULONG LimitHigh : 4;                                                  //0x4
            ULONG System : 1;                                                     //0x4
            ULONG LongMode : 1;                                                   //0x4
            ULONG DefaultBig : 1;                                                 //0x4
            ULONG Granularity : 1;                                                //0x4
            ULONG BaseHigh : 8;                                                   //0x4
        } Bits;                                                                 //0x4
        ULONG BaseUpper;                                                    //0x8
    };
    struct
    {
        ULONG MustBeZero;                                                   //0xc
        LONGLONG DataLow;                                                   //0x0
    };
    LONGLONG DataHigh;                                                      //0x8
} kgdtentry64;
