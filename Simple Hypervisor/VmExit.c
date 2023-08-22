#include "stdafx.h"

//
// VMX Basic Exit Reasons.
// **
// @see Vol3D[C(VMX BASIC EXIT REASONS)](reference)
//

VOID VmExitHandler(PVOID Param) {
	struct _GuestRegiters* guestRegisters = (struct _GuestRegiters*)Param;

	DbgPrint("[*] VM Exit Handler called .....\n");

	ULONG VmExitReason = 0;
	__vmx_vmread(VMCS_EXIT_REASON, (size_t*)&VmExitReason);
	DbgPrint("VMCS_EXIT_REASON : %x\n", VmExitReason);

	switch (VmExitReason)
	{
	case VMX_EXIT_REASON_EXCEPTION_OR_NMI: {
		break;
	}
	case VMX_EXIT_REASON_EXTERNAL_INTERRUPT: {
		break;
	}

	case VMX_EXIT_REASON_TRIPLE_FAULT: {
		break;
	}

	case VMX_EXIT_REASON_INIT_SIGNAL: {
		break;
	}

	case VMX_EXIT_REASON_STARTUP_IPI: {
		break;
	}

	case VMX_EXIT_REASON_IO_SMI: {
		break;
	}

	case VMX_EXIT_REASON_SMI: {
		break;
	}

	case VMX_EXIT_REASON_INTERRUPT_WINDOW: {
		break;
	}

	case VMX_EXIT_REASON_NMI_WINDOW: {
		break;
	}

	case VMX_EXIT_REASON_TASK_SWITCH: {
		break;
	}

	case VMX_EXIT_REASON_EXECUTE_CPUID: {
		break;
	}

	case VMX_EXIT_REASON_EXECUTE_GETSEC: {
		break;
	}

	case VMX_EXIT_REASON_EXECUTE_HLT: {
		//DbgPrint("[*] Execution of HLT detected... \n");

		//
		// Terminate HyperVisor
		//
		//HostTerminateHypervisor();
		break;
	}

	case VMX_EXIT_REASON_EXECUTE_INVD: {
		break;
	}

	case VMX_EXIT_REASON_EXECUTE_INVLPG: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_RDPMC: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_RDTSC: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_RSM_IN_SMM: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_VMCALL: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_VMCLEAR: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_VMLAUNCH: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_VMPTRLD: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_VMPTRST: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_VMREAD: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_VMRESUME: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_VMWRITE: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_VMXOFF: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_VMXON: {
		break;
	}
	case VMX_EXIT_REASON_MOV_CR: {
		break;
	}
	case VMX_EXIT_REASON_MOV_DR: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_IO_INSTRUCTION: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_RDMSR: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_WRMSR: {
		break;
	}
	case VMX_EXIT_REASON_ERROR_INVALID_GUEST_STATE: {
		break;
	}
	case VMX_EXIT_REASON_ERROR_MSR_LOAD: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_MWAIT: {
		break;
	}
	case VMX_EXIT_REASON_MONITOR_TRAP_FLAG: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_MONITOR: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_PAUSE: {
		break;
	}
	case VMX_EXIT_REASON_ERROR_MACHINE_CHECK: {
		break;
	}
	case VMX_EXIT_REASON_TPR_BELOW_THRESHOLD: {
		break;
	}
	case VMX_EXIT_REASON_APIC_ACCESS: {
		break;
	}
	case VMX_EXIT_REASON_VIRTUALIZED_EOI: {
		break;
	}
	case VMX_EXIT_REASON_GDTR_IDTR_ACCESS: {
		break;
	}
	case VMX_EXIT_REASON_LDTR_TR_ACCESS: {
		break;
	}
	case VMX_EXIT_REASON_EPT_VIOLATION: {
		break;
	}
	case VMX_EXIT_REASON_EPT_MISCONFIGURATION: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_INVEPT: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_RDTSCP: {
		break;
	}
	case VMX_EXIT_REASON_VMX_PREEMPTION_TIMER_EXPIRED: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_INVVPID: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_WBINVD: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_XSETBV: {
		break;
	}
	case VMX_EXIT_REASON_APIC_WRITE: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_RDRAND: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_INVPCID: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_VMFUNC: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_ENCLS: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_RDSEED: {
		break;
	}
	case VMX_EXIT_REASON_PAGE_MODIFICATION_LOG_FULL: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_XSAVES: {
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_XRSTORS: {
		break;
	}
	default:
		break;
	}

	return;
}