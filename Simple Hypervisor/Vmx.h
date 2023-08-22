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

VOID LaunchVm(int processorId);

VOID TerminateVm();

VOID VmExitHandler();