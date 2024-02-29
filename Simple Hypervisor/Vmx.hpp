#include "stdafx.h"

namespace vmx {
	auto VmxIsVmxAvailable() -> bool;
	auto VmxIsVmxSupport() -> bool;
	auto VmxCheckBiosLock() -> bool;
	auto VmxEnableCR4() -> void;
}

namespace hv {
	auto VirtualizeAllProcessors() -> bool;

	auto DevirtualizeAllProcessors() -> void;
}