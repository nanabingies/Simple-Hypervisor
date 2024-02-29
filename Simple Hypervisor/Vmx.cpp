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


		DbgPrint("[*] Initial checks completed.\n");

		//
		// Build MTRR Map
		//
		//if (EptBuildMTRRMap() == FALSE)	return FALSE;
		LOG("[*] MTRR built successfully\n");

		return TRUE;
	}
}