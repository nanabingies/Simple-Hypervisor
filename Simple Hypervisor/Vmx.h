#pragma once

UINT64 inline AsmSaveHostRegisters();

ULONG_PTR HostTerminateHypervisor(ULONG_PTR);

BOOLEAN IsVmxAvailable();

BOOLEAN IsVmxSupport();

VOID EnableCR4();

BOOLEAN CheckBiosLock();

BOOLEAN allocateVmcsRegion(UCHAR);

BOOLEAN allocateVmxonRegion(UCHAR);

BOOLEAN allocateVmExitStack(UCHAR);

BOOLEAN allocateIoBitmapStack(UCHAR);

BOOLEAN allocateMsrBitmap(UCHAR);

BOOLEAN VirtualizeAllProcessors();

VOID DevirtualizeAllProcessors();

ULONG_PTR LaunchVm(_In_ ULONG_PTR Argument);

VOID TerminateVm();

VOID VmExitHandler(PVOID);