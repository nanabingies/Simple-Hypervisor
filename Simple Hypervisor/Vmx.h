#pragma once

VOID SaveHostRegisters(UINT64, UINT64);
ULONG_PTR HostTerminateHypervisor(ULONG_PTR);

BOOLEAN IsVmxSupport();

VOID EnableCR4();

BOOLEAN CheckBiosLock();

BOOLEAN allocateVmcsRegion(UCHAR);

BOOLEAN allocateVmxonRegion(UCHAR);

BOOLEAN allocateVmExitStack(UCHAR);

BOOLEAN allocateIoBitmapStack(UCHAR);

BOOLEAN VirtualizeAllProcessors();

VOID DevirtualizeAllProcessors();

VOID LaunchVm(_In_ struct _KDPC* Dpc, _In_opt_ PVOID DeferredContext, _In_opt_ PVOID, _In_opt_ PVOID);

VOID TerminateVm();

VOID VmExitHandler(PVOID);