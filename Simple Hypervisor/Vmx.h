#pragma once

BOOLEAN IsVmxSupport();

VOID EnableCR4();

BOOLEAN CheckBiosLock();

VOID allocateVmcsRegion(UCHAR processorNumber);

VOID allocateVmxonRegion(UCHAR processorNumber);

VOID VirtualizeAllProcessors();

VOID DevirtualizeAllProcessors();