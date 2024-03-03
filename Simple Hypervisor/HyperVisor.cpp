#include "vmx.hpp"

namespace hv {
	auto virtualizeAllProcessors() -> bool {
		PAGED_CODE();

		//
		// This was more of an educational project so only one Logical Processor was chosen and virtualized
		// TODO : Add support for multiple processors
		// Update: Support for multiple processors added
		//

		PROCESSOR_NUMBER processor_number;
		GROUP_AFFINITY affinity, old_affinity;

		//
		// ExAllocatePool2 - Memory is zero initialized unless POOL_FLAG_UNINITIALIZED is specified.
		//
		vmm_context = reinterpret_cast<_vmm_context*>
			(ExAllocatePoolWithTag(NonPagedPool, sizeof(_vmm_context) * g_num_processors, VMM_POOL_TAG);

		for (unsigned iter = 0; iter < g_num_processors; iter++) {

		}
	}
}