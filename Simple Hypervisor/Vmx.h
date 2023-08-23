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

ULONG_PTR LaunchVm(_In_ ULONG_PTR);

VOID TerminateVm();

VOID VmExitHandler(PVOID);