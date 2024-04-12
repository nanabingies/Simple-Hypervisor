#include "stdafx.h"
#pragma warning(disable: 4996)

namespace hv {
	auto launch_vm() -> void {
		auto ret = asm_hv_launch();
		if (ret != 0) {		// Failure
			LOG("[!] Failed to lauch vm on processor (%x)\n", KeGetCurrentProcessorNumber());
			LOG_ERROR(__FILE__, __LINE__);
			return;
		}
	}

	auto resume_vm() -> void {
		//UNREFERENCED_PARAMETER(processor_number);

		size_t Rip, InstLen;
		__vmx_vmread(VMCS_GUEST_RIP, &Rip);
		__vmx_vmread(VMCS_VMEXIT_INSTRUCTION_LENGTH, &InstLen);

		Rip += InstLen;
		__vmx_vmwrite(VMCS_GUEST_RIP, Rip);

		//
		// the VMRESUME instruction requires a VMCS whose launch state is set to launched .
		// we're still in launch mode
		//
		__vmx_vmresume();

		return;
	}

}
