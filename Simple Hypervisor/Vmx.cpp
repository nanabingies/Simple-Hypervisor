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
		if (!VmxCheckBiosLock())	return FALSE;

		//
		// Enable VMXE for all processors
		//
		VmxEnableCR4();

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

		for (auto idx = 0; idx < g_num_processors; ++idx) {
			LOG("[*] Checking processor %x VMX support.....\n", idx);
			cpuid_eax_01 args{};
			__cpuid(reinterpret_cast<int32_t*>(&args), 1);

			if (args.cpuid_feature_information_ecx.virtual_machine_extensions == 0) {
				LOG_ERROR("[-] This processor does not support VMX Extensions.\n");
				return false;
			}
		}
	}
}
