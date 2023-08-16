#pragma once

VOID SaveHostRegisters();
VOID HostTerminateHypervisor();

BOOLEAN IsVmxSupport();

VOID EnableCR4();

BOOLEAN CheckBiosLock();

VOID allocateVmcsRegion(UCHAR processorNumber);

VOID allocateVmxonRegion(UCHAR processorNumber);

VOID VirtualizeAllProcessors();

VOID DevirtualizeAllProcessors();

VOID LaunchVm(int processorId);

VOID TerminateVm();

VOID VmExitHandler();