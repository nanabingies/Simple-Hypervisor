#include "stdafx.h"
//#include "logger.hpp"
//#include "vmutils.hpp"

namespace vmx {
	auto vmxIsVmxAvailable() -> bool;
	auto vmxIsVmxSupport() -> bool;
	auto vmxCheckBiosLock() -> bool;
	auto vmxEnableCR4() -> void;
}

namespace hv {
	auto virtualizeAllProcessors() -> bool;

	auto devirtualizeAllProcessors() -> void;
}
