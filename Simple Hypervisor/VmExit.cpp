#include "stdafx.h"

namespace vmexit {
	auto vmexit_handler(void* _guest_registers) -> void {
		auto guest_regs = reinterpret_cast<guest_registers*>(_guest_registers);
		if (!guest_regs)	return;
	}
}