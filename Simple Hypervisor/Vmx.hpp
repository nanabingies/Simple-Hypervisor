#include "stdafx.h"

namespace vmx {
	auto VmxIsVmxAvailable() -> bool;
}

namespace hv {
	auto VirtualizeAllProcessors() -> bool;

	auto DevirtualizeAllProcessors() -> void;
}