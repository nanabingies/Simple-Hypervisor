#pragma once

VOID SaveHostRegisters();
VOID HostTerminateHypervisor();

BOOLEAN IsVmxSupport();

VOID EnableCR4();

BOOLEAN CheckBiosLock();

BOOLEAN allocateVmcsRegion(UCHAR processorNumber);

BOOLEAN allocateVmxonRegion(UCHAR processorNumber);

BOOLEAN allocateVmExitStack(UCHAR processorNumber);

BOOLEAN allocateMsrStack(UCHAR processorNumber);

BOOLEAN VirtualizeAllProcessors();

VOID DevirtualizeAllProcessors();

ULONG_PTR LaunchVm(_In_ ULONG_PTR);

VOID TerminateVm();

VOID VmExitHandler();