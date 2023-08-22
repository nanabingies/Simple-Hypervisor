#include "stdafx.h"

//
// VMX Basic Exit Reasons.
// **
// @see Vol3D[C(VMX BASIC EXIT REASONS)](reference)
//

VOID VmExitHandler(PVOID Param) {
	struct _GuestRegisters* guestRegisters = (struct _GuestRegisters*)Param;

	VMX_VMEXIT_REASON VmExitInfo;
	__vmx_vmread(VMCS_EXIT_REASON, (size_t*) & VmExitInfo);
	DbgPrint("VMCS_EXIT_REASON : %lx\n", VmExitInfo.BasicExitReason);

	switch (VmExitInfo.BasicExitReason)
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
		VMX_EXIT_QUALIFICATION_TASK_SWITCH exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*) & exitQualification);
		break;
	}

	case VMX_EXIT_REASON_EXECUTE_CPUID: {
		int cpuInfo[4] = { 0 };

		//
		// Terminate VM ??
		//
		if (guestRegisters->RAX == 0x6e616e61 && guestRegisters->RCX == 0x6e616e61) {	// 'nana'
			__vmx_vmread(VMCS_GUEST_RIP, &g_GuestRip);
			__vmx_vmread(VMCS_GUEST_RSP, &g_GuestRsp);

			size_t len = 0;
			__vmx_vmread(VMCS_VMEXIT_INSTRUCTION_LENGTH, &len);
			g_GuestRip += len;
			// now jump tp g_GuestRip
		}

		__cpuidex((int*)&cpuInfo, (int)guestRegisters->RAX, (int)guestRegisters->RCX);
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
		VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*) & exitQualification);
		break;
	}
	case VMX_EXIT_REASON_EXECUTE_VMPTRST: {
		VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*) & exitQualification);
		break;
	}

	case VMX_EXIT_REASON_EXECUTE_VMREAD: {
		VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*) & exitQualification);
		break;
	}

	case VMX_EXIT_REASON_EXECUTE_VMRESUME: {
		break;
	}

	case VMX_EXIT_REASON_EXECUTE_VMWRITE: {
		VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*) & exitQualification);
		break;
	}

	case VMX_EXIT_REASON_EXECUTE_VMXOFF: {
		break;
	}

	case VMX_EXIT_REASON_EXECUTE_VMXON: {
		VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*) & exitQualification);
		break;
	}

	case VMX_EXIT_REASON_MOV_CR: {
		//
		// Check whether it was a mov to or mov from CR
		//
		VMX_EXIT_QUALIFICATION_MOV_CR exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*) & exitQualification);
		switch (exitQualification.AccessType) {
		case 0: {	// MOV to CR
			switch (exitQualification.ControlRegister) {
			case 0: {	// CR0
				__vmx_vmwrite(VMCS_GUEST_CR0, __readcr0());
				__vmx_vmwrite(VMCS_CTRL_CR0_READ_SHADOW, __readcr0());
				break;
			}

			case 3: {	// CR3
				__vmx_vmwrite(VMCS_GUEST_CR3, __readcr3());
				break;
			}

			case 4: {	// CR4
				__vmx_vmwrite(VMCS_GUEST_CR4, __readcr4());
				__vmx_vmwrite(VMCS_CTRL_CR4_READ_SHADOW, __readcr4());
				break;
			}

			default:
				break;
			}
			break;
		}
		case 1: {	// MOV from CR
			switch (exitQualification.ControlRegister) {
			case 0: {		// CR0
				__vmx_vmread(VMCS_GUEST_CR0, &guestRegisters->RCX);
				break;
			}

			case 3: {		// CR3
				__vmx_vmread(VMCS_GUEST_CR3, &guestRegisters->RCX);
				break;
			}

			case 4: {		// CR4
				__vmx_vmread(VMCS_GUEST_CR4, &guestRegisters->RCX);
				break;
			}
			}
			break;
		}
		case 2: {	// 2 = CLTS
			break;
		}
		case 3: {	// 3 = LMSW
			break;
		}
		default: {
			break;
		}
		}
		
		break;
	}

	case VMX_EXIT_REASON_MOV_DR: {
		VMX_EXIT_QUALIFICATION_MOV_DR exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*) &exitQualification);
		break;
	}

	case VMX_EXIT_REASON_EXECUTE_IO_INSTRUCTION: {
		VMX_EXIT_QUALIFICATION_IO_INSTRUCTION exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*) &exitQualification);
		break;
	}

	case VMX_EXIT_REASON_EXECUTE_RDMSR: {
		if ((guestRegisters->RCX <= 0x00001FFF) ||
			((guestRegisters->RCX >= 0xC0000000) && (guestRegisters->RCX <= 0xC0001FFF))) {

			LARGE_INTEGER msr;
			msr.QuadPart = __readmsr((ULONG)guestRegisters->RCX);
			guestRegisters->RDX = msr.HighPart;
			guestRegisters->RAX = msr.LowPart;
		}

		break;
	}

	case VMX_EXIT_REASON_EXECUTE_WRMSR: {
		if ((guestRegisters->RCX <= 0x00001FFF) ||
			((guestRegisters->RCX >= 0xC0000000) && (guestRegisters->RCX <= 0xC0001FFF))) {
			
			LARGE_INTEGER msr;
			msr.LowPart = (ULONG)guestRegisters->RAX;
			msr.HighPart = (ULONG)guestRegisters->RDX;
			__writemsr((ULONG)guestRegisters->RCX, msr.QuadPart);
		}

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
		VMX_EXIT_QUALIFICATION_APIC_ACCESS exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*) & exitQualification);
		break;
	}

	case VMX_EXIT_REASON_VIRTUALIZED_EOI: {
		break;
	}

	case VMX_EXIT_REASON_GDTR_IDTR_ACCESS: {
		VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*) & exitQualification);
		break;
	}

	case VMX_EXIT_REASON_LDTR_TR_ACCESS: {
		VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*) & exitQualification);
		break;
	}

	case VMX_EXIT_REASON_EPT_VIOLATION: {
		VMX_EXIT_QUALIFICATION_EPT_VIOLATION exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*) & exitQualification);
		break;
	}

	case VMX_EXIT_REASON_EPT_MISCONFIGURATION: {
		break;
	}

	case VMX_EXIT_REASON_EXECUTE_INVEPT: {
		VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*) & exitQualification);
		break;
	}

	case VMX_EXIT_REASON_EXECUTE_RDTSCP: {
		break;
	}

	case VMX_EXIT_REASON_VMX_PREEMPTION_TIMER_EXPIRED: {
		break;
	}

	case VMX_EXIT_REASON_EXECUTE_INVVPID: {
		VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*) & exitQualification);
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
		VMX_VMEXIT_INSTRUCTION_INFO_RDRAND_RDSEED exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*) & exitQualification);
		break;
	}

	case VMX_EXIT_REASON_EXECUTE_INVPCID: {
		VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*) & exitQualification);
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
		VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*) & exitQualification);
		break;
	}

	case VMX_EXIT_REASON_EXECUTE_XRSTORS: {
		VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*) & exitQualification);
		break;
	}

	default:
		break;
	}

	return;
}