#include "stdafx.h"

VOID VmExitHandler() {
	DbgPrint("[*] VM Exit Handler called .....\n");

	ULONG VmExitReason = 0;
	__vmx_vmread(VMCS_EXIT_REASON, (size_t*)&VmExitReason);
	DbgPrint("VMCS_EXIT_REASON : %x\n", VmExitReason);

	switch (VmExitReason)
	{
	case VMX_EXIT_REASON_EXECUTE_HLT: {
		DbgPrint("[*] Execution of HLT detected... \n");

		//
		// Terminate HyperVisor
		//
		HostTerminateHypervisor();
	}
	default:
		break;
	}

	return;
}