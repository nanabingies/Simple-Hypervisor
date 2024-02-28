#include "stdafx.h"

//
// VMX Basic Exit Reasons.
// **
// @see Vol3D[C(VMX BASIC EXIT REASONS)](reference)
//

auto VmExitHandler(void* Param) -> void {
	GuestRegisters* guestRegisters = (GuestRegisters*)Param;

	vmx_vmexit_reason VmExitReason;
	__vmx_vmread(VMCS_EXIT_REASON, reinterpret_cast<size_t*>(&VmExitReason));
	//DbgPrint("[*] VMCS_EXIT_REASON : %lx\n", VmExitInfo.BasicExitReason);
	//DbgPrint("[*] This came from processor : %lx\n", KeGetCurrentNodeNumber());

	switch (VmExitReason.basic_exit_reason)
	{
	case VMX_EXIT_REASON_EXCEPTION_OR_NMI: {
		DbgPrint("[*] exception or nmi\n");
	}
										 break;

	case VMX_EXIT_REASON_EXTERNAL_INTERRUPT: {
		DbgPrint("[*] external interrupt\n");
	}
										   break;

	case VMX_EXIT_REASON_TRIPLE_FAULT: {
		DbgPrint("[*] triple fault\n");
	}
									 break;

	case VMX_EXIT_REASON_INIT_SIGNAL: {
		DbgPrint("[*] init signal\n");
	}
									break;

	case VMX_EXIT_REASON_STARTUP_IPI: {
		DbgPrint("[*] startup ipi\n");
	}
									break;

	case VMX_EXIT_REASON_IO_SMI: {
		DbgPrint("[*] io smi\n");
	}
							   break;

	case VMX_EXIT_REASON_SMI: {
		DbgPrint("[*] smi\n");
	}
							break;

	case VMX_EXIT_REASON_INTERRUPT_WINDOW: {
		DbgPrint("[*] interrupt window\n");
	}
										 break;

	case VMX_EXIT_REASON_NMI_WINDOW: {
		DbgPrint("[*] nmi window\n");
	}
								   break;

	case VMX_EXIT_REASON_TASK_SWITCH: {
		DbgPrint("[*] task switch\n");
		vmx_exit_qualification_task_switch exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}
									break;

	case VMX_EXIT_REASON_EXECUTE_CPUID: {
		DbgPrint("[*] execute cpuid\n");
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
	}
									  break;

	case VMX_EXIT_REASON_EXECUTE_GETSEC: {
		DbgPrint("[*] execute getsec\n");
	}
									   break;

	case VMX_EXIT_REASON_EXECUTE_HLT: {
		DbgPrint("[*] execute hlt\n");
		
		//
		// Terminate HyperVisor
		//
		//HostTerminateHypervisor();
	}
									break;

	case VMX_EXIT_REASON_EXECUTE_INVD: {
		DbgPrint("[*] execute invd\n");
	}
									 break;

	case VMX_EXIT_REASON_EXECUTE_INVLPG: {
		DbgPrint("[*] execute invlpg\n");
	}
									   break;

	case VMX_EXIT_REASON_EXECUTE_RDPMC: {
		DbgPrint("[*] execute rdpmc\n");
	}
									  break;

	case VMX_EXIT_REASON_EXECUTE_RDTSC: {
		DbgPrint("[*] execute rdtsc\n");
	}
									  break;

	case VMX_EXIT_REASON_EXECUTE_RSM_IN_SMM: {
		DbgPrint("[*] execute rsm in smm\n");
	}
										   break;

	case VMX_EXIT_REASON_EXECUTE_VMCALL: {
		DbgPrint("[*] execute vmcall\n");
	}
									   break;

	case VMX_EXIT_REASON_EXECUTE_VMCLEAR: {
		DbgPrint("[*] execute vmclear\n");
	}
										break;

	case VMX_EXIT_REASON_EXECUTE_VMLAUNCH: {
		DbgPrint("[*] execute vmlaunch\n");
	}
										 break;

	case VMX_EXIT_REASON_EXECUTE_VMPTRLD: {
		DbgPrint("[*] execute vmptrld\n");
		vmx_vmexit_instruction_info_vmx_and_xsaves exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}
										break;

	case VMX_EXIT_REASON_EXECUTE_VMPTRST: {
		DbgPrint("[*] execute vmptrst\n");
		vmx_vmexit_instruction_info_vmx_and_xsaves exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}
										break;

	case VMX_EXIT_REASON_EXECUTE_VMREAD: {
		DbgPrint("[*] execute vmread\n");
		vmx_vmexit_instruction_info_vmread_vmwrite exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}
									   break;

	case VMX_EXIT_REASON_EXECUTE_VMRESUME: {
		DbgPrint("[*] execute vmresume\n");
	}
										 break;

	case VMX_EXIT_REASON_EXECUTE_VMWRITE: {
		DbgPrint("[*] execute vmwrite\n");
		vmx_vmexit_instruction_info_vmread_vmwrite exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}
										break;

	case VMX_EXIT_REASON_EXECUTE_VMXOFF: {
		DbgPrint("[*] execute vmxoff\n");
	}
									   break;

	case VMX_EXIT_REASON_EXECUTE_VMXON: {
		DbgPrint("[*] execute vmxon\n");
		vmx_vmexit_instruction_info_vmx_and_xsaves exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}
									  break;

	case VMX_EXIT_REASON_MOV_CR: {
		DbgPrint("[*] mov cr\n");

		//
		// Check whether it was a mov to or mov from CR
		//
		vmx_exit_qualification_mov_cr exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*) & exitQualification);
		switch (exitQualification.access_type) {
		case 0: {	// MOV to CR
			switch (exitQualification.control_register) {
			case 0: {	// CR0
				__vmx_vmwrite(VMCS_GUEST_CR0, __readcr0());
				__vmx_vmwrite(VMCS_CTRL_CR0_READ_SHADOW, __readcr0());
			}
				  break;

			case 3: {	// CR3
				__vmx_vmwrite(VMCS_GUEST_CR3, __readcr3());
			}
				  break;

			case 4: {	// CR4
				__vmx_vmwrite(VMCS_GUEST_CR4, __readcr4());
				__vmx_vmwrite(VMCS_CTRL_CR4_READ_SHADOW, __readcr4());
			}
				  break;

			default:
				break;
			}
		}
			  break;

		case 1: {	// MOV from CR
			switch (exitQualification.control_register) {
			case 0: {		// CR0
				__vmx_vmread(VMCS_GUEST_CR0, &guestRegisters->RCX);
			}
				  break;

			case 3: {		// CR3
				__vmx_vmread(VMCS_GUEST_CR3, &guestRegisters->RCX);
			}
				  break;

			case 4: {		// CR4
				__vmx_vmread(VMCS_GUEST_CR4, &guestRegisters->RCX);
			}
				  break;
			}
		}
			  break;

		case 2: {	// 2 = CLTS
			
		}
			  break;

		case 3: {	// 3 = LMSW
			
		}
			  break;

		default:
			break;
		}
		
	}
							   break;

	case VMX_EXIT_REASON_MOV_DR: {
		DbgPrint("[*] mov dr\n");
		vmx_exit_qualification_mov_dr exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}
							   break;

	case VMX_EXIT_REASON_EXECUTE_IO_INSTRUCTION: {
		DbgPrint("[*] execute io\n");
		vmx_exit_qualification_io_instruction exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}
											   break;

	case VMX_EXIT_REASON_EXECUTE_RDMSR: {
		DbgPrint("[*] execute rdmsr\n");
		if ((guestRegisters->RCX <= 0x00001FFF) ||
			((guestRegisters->RCX >= 0xC0000000) && (guestRegisters->RCX <= 0xC0001FFF))) {

			LARGE_INTEGER msr;
			msr.QuadPart = __readmsr(static_cast<ulong>(guestRegisters->RCX));
			guestRegisters->RDX = msr.HighPart;
			guestRegisters->RAX = msr.LowPart;
		}
	}
									  break;

	case VMX_EXIT_REASON_EXECUTE_WRMSR: {
		DbgPrint("[*] execute wrmsr\n");
		if ((guestRegisters->RCX <= 0x00001FFF) ||
			((guestRegisters->RCX >= 0xC0000000) && (guestRegisters->RCX <= 0xC0001FFF))) {
			
			LARGE_INTEGER msr;
			msr.LowPart = static_cast<ulong>(guestRegisters->RAX);
			msr.HighPart = static_cast<ulong>(guestRegisters->RDX);
			__writemsr(static_cast<ulong>(guestRegisters->RCX), msr.QuadPart);
		}
	
	}
									  break;

	case VMX_EXIT_REASON_ERROR_INVALID_GUEST_STATE: {
		DbgPrint("[*] invalid guest state\n");
	}
												  break;

	case VMX_EXIT_REASON_ERROR_MSR_LOAD: {
		DbgPrint("[*] error msr load\n");
	}
									   break;

	case VMX_EXIT_REASON_EXECUTE_MWAIT: {
		DbgPrint("[*] execute mwait\n");
	}
									  break;

	case VMX_EXIT_REASON_MONITOR_TRAP_FLAG: {
		DbgPrint("[*] monitor trap flag\n");
	}
										  break;

	case VMX_EXIT_REASON_EXECUTE_MONITOR: {
		DbgPrint("[*] execute monitor\n");
	}
										break;

	case VMX_EXIT_REASON_EXECUTE_PAUSE: {
		DbgPrint("[*] execute pause\n");
	}
									  break;

	case VMX_EXIT_REASON_ERROR_MACHINE_CHECK: {
		DbgPrint("[*] error machine check\n");
	}
											break;

	case VMX_EXIT_REASON_TPR_BELOW_THRESHOLD: {
		DbgPrint("[*] below threshold\n");
	}
											break;

	case VMX_EXIT_REASON_APIC_ACCESS: {
		DbgPrint("[*] apic access\n");
		vmx_exit_qualification_apic_access exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}
									break;

	case VMX_EXIT_REASON_VIRTUALIZED_EOI: {
		DbgPrint("[*] virtualized eoi\n");
	}
										break;

	case VMX_EXIT_REASON_GDTR_IDTR_ACCESS: {
		DbgPrint("[*] gdtr idtr access\n");
		vmx_vmexit_instruction_info_gdtr_idtr_access exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}
										 break;

	case VMX_EXIT_REASON_LDTR_TR_ACCESS: {
		DbgPrint("[*] ldtr tr access\n");
		vmx_vmexit_instruction_info_ldtr_tr_access exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}
									   break;

	case VMX_EXIT_REASON_EPT_VIOLATION: {
		DbgPrint("[*] ept violation\n");

		vmx_exit_qualification_ept_violation exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
		
		UINT64 phys_addr;
		__vmx_vmread(VMCS_GUEST_PHYSICAL_ADDRESS, &phys_addr);
		//phys_addr = PAGE_ALIGN((PVOID)phys_addr);
		

		UINT64 linear_addr;
		__vmx_vmread(VMCS_EXIT_GUEST_LINEAR_ADDRESS, &linear_addr);

		if (exitQualification.ept_executable || exitQualification.ept_readable || exitQualification.ept_writeable) {
			__debugbreak();
			DbgPrint("Error: VA = %llx, PA = %llx", linear_addr, phys_addr);
			return;
		}

		//HandleEptViolation(phys_addr, linear_addr);
	}
									  break;

	case VMX_EXIT_REASON_EPT_MISCONFIGURATION: {
		DbgPrint("[*] EPT Misconfiguration\n");
		__debugbreak();

		// Failure setting EPT
		// Bugcheck and restart system
		KeBugCheck(PFN_LIST_CORRUPT);	// Is this bug code even correct??
	}
											 break;

	case VMX_EXIT_REASON_EXECUTE_INVEPT: {
		DbgPrint("[*] invept\n");
		vmx_vmexit_instruction_info_invalidate exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
		__debugbreak();
	}
									   break;

	case VMX_EXIT_REASON_EXECUTE_RDTSCP: {
		DbgPrint("[*] rdtscp\n");
	}
									   break;

	case VMX_EXIT_REASON_VMX_PREEMPTION_TIMER_EXPIRED: {
		DbgPrint("[*] timer expired\n");
	}
													 break;

	case VMX_EXIT_REASON_EXECUTE_INVVPID: {
		DbgPrint("[*] invvpid\n");
		vmx_vmexit_instruction_info_invalidate exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}
										break;

	case VMX_EXIT_REASON_EXECUTE_WBINVD: {
		DbgPrint("[*] wbinvd\n");
	}
									   break;

	case VMX_EXIT_REASON_EXECUTE_XSETBV: {
		DbgPrint("[*] xsetvb\n");
	}
									   break;

	case VMX_EXIT_REASON_APIC_WRITE: {
		DbgPrint("[*] apic write\n");
	}
								   break;

	case VMX_EXIT_REASON_EXECUTE_RDRAND: {
		DbgPrint("[*] rdrand\n");
		vmx_vmexit_instruction_info_rdrand_rdseed exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}
									   break;

	case VMX_EXIT_REASON_EXECUTE_INVPCID: {
		DbgPrint("[*] invpcid\n");
		vmx_vmexit_instruction_info_invalidate exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}
										break;

	case VMX_EXIT_REASON_EXECUTE_VMFUNC: {
		DbgPrint("[*] vmfunc\n");
	}
									   break;

	case VMX_EXIT_REASON_EXECUTE_ENCLS: {
		DbgPrint("[*] execute encls\n");
	}
									  break;

	case VMX_EXIT_REASON_EXECUTE_RDSEED: {
		DbgPrint("[*] execute rdseed\n");
	}
									   break;

	case VMX_EXIT_REASON_PAGE_MODIFICATION_LOG_FULL: {
		DbgPrint("[*] modification log full\n");
	}
												   break;

	case VMX_EXIT_REASON_EXECUTE_XSAVES: {
		DbgPrint("[*] execute xsaves\n");
		vmx_vmexit_instruction_info_vmx_and_xsaves exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}
									   break;

	case VMX_EXIT_REASON_EXECUTE_XRSTORS: {
		DbgPrint("[*] execute xstors\n");
		vmx_vmexit_instruction_info_vmx_and_xsaves exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}
										break;

	default:
		break;
	}

	return;
}
