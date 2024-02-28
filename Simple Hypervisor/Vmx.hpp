#pragma once
#include "stdafx.h"

//extern "C" auto inline AsmSaveHostRegisters() -> uint64_t;

auto HostTerminateHypervisor(ulong_ptr);

auto VmxIsVmxAvailable() -> bool;

auto VmxIsVmxSupport() -> bool;

auto VmxEnableCR4() -> void;

auto VmxCheckBiosLock() -> bool;

auto VmxAllocateVmcsRegion(uchar) -> bool;

auto VmxAllocateVmxonRegion(uchar) -> bool;

auto VmxAllocateVmExitStack(uchar) -> bool;

auto VmxAllocateIoBitmapStack(uchar) -> bool;

auto VmxAllocateMsrBitmap(uchar) -> bool;

auto VirtualizeAllProcessors() -> bool;

auto DevirtualizeAllProcessors() -> void;

auto LaunchVm(ulong_ptr) -> ulong_ptr;

auto TerminateVm() -> void;

auto VmExitHandler(void*) -> void;
