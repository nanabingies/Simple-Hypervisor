#pragma once

VOID SaveHostRegisters();
VOID HostTerminateHypervisor();

BOOLEAN IsVmxSupport();

VOID EnableCR4();

BOOLEAN CheckBiosLock();

BOOLEAN allocateVmcsRegion(UCHAR);

BOOLEAN allocateVmxonRegion(UCHAR);

BOOLEAN allocateVmExitStack(UCHAR);

BOOLEAN allocateMsrStack(UCHAR);

BOOLEAN VirtualizeAllProcessors();

VOID DevirtualizeAllProcessors();

ULONG_PTR LaunchVm(_In_ ULONG_PTR);

VOID TerminateVm();

VOID VmExitHandler(PVOID);