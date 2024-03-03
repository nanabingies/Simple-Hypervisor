#include "vmx.hpp"
using ept::checkEPTSupport;

namespace vmx {
	auto vmxIsVmxAvailable() -> bool {
		PAGED_CODE();

		//
		// Check VMX Support for all Logical Processors
		//
		if (!vmxIsVmxSupport())	return false;

		//
		// Check Bios Lock Bit
		//
		if (!vmxCheckBiosLock())	return false;

		//
		// Enable VMXE for all processors
		//
		vmxEnableCR4();

		//
		// Check for EPT support for all processors
		//
		if (!checkEPTSupport())	return false;


		LOG("[*] Initial checks completed.\n");

		//
		// Build MTRR Map
		//
		//if (EptBuildMTRRMap() == FALSE)	return FALSE;
		LOG("[*] MTRR built successfully\n");

		return TRUE;
	}

	auto vmxIsVmxSupport() -> bool {
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

			LOG("[*] Checking processor (%x) VMX support.....\n", processor_number.Number);
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

	auto vmxCheckBiosLock() -> bool {
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

			ia32_feature_control_register feature_control_msr{};
			feature_control_msr.flags = __readmsr(IA32_FEATURE_CONTROL);

			if (feature_control_msr.lock_bit == 0) {
				// Enable it
				feature_control_msr.lock_bit = 1;
				feature_control_msr.enable_vmx_outside_smx = 1;

				__writemsr(IA32_FEATURE_CONTROL, feature_control_msr.flags);
			}

			if (feature_control_msr.enable_vmx_outside_smx == 0) {
				LOG("[-] EnableVmxOutsideSmx bit not set.\n");
				LOG_ERROR();
				return FALSE;
			}

			KeLowerIrql(irql);
			KeRevertToUserGroupAffinityThread(&old_affinity);
		}

		return true;
	}

	auto vmxEnableCR4() -> void {
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

			PAGED_CODE();

			cr4 _cr4;

			_cr4.flags = __readcr4();
			_cr4.vmx_enable = 1;

			__writecr4(_cr4.flags);

			KeLowerIrql(irql);
			KeRevertToUserGroupAffinityThread(&old_affinity);
		}

		return;
	}
}
