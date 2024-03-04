#include "stdafx.h"
//#include "logger.hpp"
//#include "vmutils.hpp"
using uchar = unsigned char;

namespace vmx {
	auto vmxIsVmxAvailable() -> bool;
	auto vmxIsVmxSupport() -> bool;
	auto vmxCheckBiosLock() -> bool;
	auto vmxEnableCR4() -> void;

	auto vmxAllocateVmxonRegion(uchar) -> bool;
	auto vmxAllocateVmcsRegion(uchar) -> bool;
	auto vmxAllocateVmExitStack(uchar) -> bool;
	auto vmxAllocateIoBitmapStack(uchar) -> bool;
	auto vmxAllocateMsrBitmap(uchar) -> bool;
}

namespace hv {
	auto virtualizeAllProcessors() -> bool;

	auto devirtualizeAllProcessors() -> void;

	auto launchVM(unsigned __int64) -> unsigned __int64;
}
