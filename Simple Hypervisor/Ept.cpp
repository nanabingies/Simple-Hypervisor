#include "ept.hpp"

namespace ept {
	static mtrr_entry g_MtrrEntries[numMtrrEntries];

	auto check_ept_support() -> bool {
		PAGED_CODE();

		PROCESSOR_NUMBER processor_number;
		GROUP_AFFINITY affinity, old_affinity;

		for (unsigned iter = 0; iter < g_num_processors; iter++) {
			KeGetProcessorNumberFromIndex(iter, &processor_number);

			RtlSecureZeroMemory(&affinity, sizeof(GROUP_AFFINITY));
			affinity.Group = processor_number.Group;
			affinity.Mask = static_cast<KAFFINITY>(1) << processor_number.Number;
			KeSetSystemGroupAffinityThread(&affinity, &old_affinity);

			auto irql = KeRaiseIrqlToDpcLevel();

			ia32_vmx_ept_vpid_cap_register ept_cap;
			ept_cap.flags = __readmsr(IA32_VMX_EPT_VPID_CAP);

			if (!ept_cap.page_walk_length_4 || !ept_cap.memory_type_write_back || !ept_cap.invept ||
				!ept_cap.invept_single_context || !ept_cap.invept_all_contexts || !ept_cap.invvpid ||
				!ept_cap.invvpid_individual_address || !ept_cap.invvpid_all_contexts ||
				!ept_cap.invvpid_single_context || !ept_cap.invvpid_single_context_retain_globals) {
				return false;
			}

			KeLowerIrql(irql);
			KeRevertToUserGroupAffinityThread(&old_affinity);
		}

		return true;
	}

	auto IsInRange(unsigned __int64 val, unsigned __int64 start, unsigned __int64 end) -> bool {
		if (val > start && val <= end)
			return true;

		return false;
	}
}
