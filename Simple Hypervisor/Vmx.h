#pragma once

UINT64 inline AsmSaveHostRegisters();

ULONG_PTR HostTerminateHypervisor(ULONG_PTR);

BOOLEAN VmxIsVmxAvailable();

BOOLEAN VmxIsVmxSupport();

VOID VmxEnableCR4();

BOOLEAN VmxCheckBiosLock();

BOOLEAN VmxAllocateVmcsRegion(UCHAR);

BOOLEAN VmxAllocateVmxonRegion(UCHAR);

BOOLEAN VmxAllocateVmExitStack(UCHAR);

BOOLEAN VmxAllocateIoBitmapStack(UCHAR);

BOOLEAN VmxAllocateMsrBitmap(UCHAR);

BOOLEAN VirtualizeAllProcessors();

VOID DevirtualizeAllProcessors();

ULONG_PTR LaunchVm(_In_ ULONG_PTR Argument);

VOID TerminateVm();

VOID VmExitHandler(PVOID);