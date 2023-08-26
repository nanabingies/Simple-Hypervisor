#pragma once

UINT64 SaveHostRegisters();

ULONG_PTR HostTerminateHypervisor(ULONG_PTR);

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

VOID LaunchVm(struct _KDPC* Dpc, PVOID DeferredContext, PVOID, PVOID);

VOID TerminateVm();

VOID VmExitHandler(PVOID);