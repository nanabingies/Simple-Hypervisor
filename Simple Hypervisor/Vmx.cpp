#include "vmx.hpp"

namespace vmx {
	auto VmxIsVmxAvailable() -> bool {
		PAGED_CODE();

		//
		// Check VMX Support for all Logical Processors
		//
		if (!VmxIsVmxSupport())	return false;

		//
		// Check Bios Lock Bit
		//
		//if (!VmxCheckBiosLock())	return FALSE;

		//
		// Enable VMXE for all processors
		//
		//VmxEnableCR4();

		//
		// Check for EPT support for all processors
		//
		//if (CheckEPTSupport() == FALSE)	return FALSE;


		LOG("[*] Initial checks completed.\n");

		//
		// Build MTRR Map
		//
		//if (EptBuildMTRRMap() == FALSE)	return FALSE;
		LOG("[*] MTRR built successfully\n");

		return TRUE;
	}

	auto VmxIsVmxSupport() -> bool {
		PAGED_CODE();

		PROCESSOR_NUMBER processor_number;
		GROUP_AFFINITY affinity, old_affinity;

		for (unsigned iter = 0; iter <g_num_processors; iter++) {
			KeGetProcessorNumberFromIndex(iter, &processor_number);

			RtlSecureZeroMemory(&affinity, sizeof(GROUP_AFFINITY));
			affinity.Group = processor_number.Group;
			affinity.Mask = (KAFFINITY)1 << processor_number.Number;
			KeSetSystemGroupAffinityThread(&affinity, &old_affinity);

			KIRQL irql = KeRaiseIrqlToDpcLevel();

			LOG("[*] Checking processor (%x) VMX support.....\n", iter);
			cpuid_eax_01 args{};
			__cpuid(reinterpret_cast<int32_t*>(&args), 1);

			if (args.cpuid_feature_information_ecx.virtual_machine_extensions == 0) {
				LOG("[-] This processor does not support VMX Extensions.\n");
				LOG_ERROR();
				return false;
			}

			KeLowerIrql(irql);
			KeRevertToUserGroupAffinityThread(&old_affinity);
		}

		return true;
	}
}
