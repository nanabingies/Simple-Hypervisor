#include "stdafx.h"

namespace vmexit {
	auto vmexit_handler(void* _guest_registers) -> void {
		auto guest_regs = reinterpret_cast<guest_registers*>(_guest_registers);
		if (!guest_regs)	return;

		vmx_vmexit_reason vmexit_reason;
		__vmx_vmread(VMCS_EXIT_REASON, reinterpret_cast<size_t*>(&vmexit_reason));

		auto current_processor = KeGetCurrentNodeNumber();
		auto current_vmm_context = vmm_context[current_processor];

		// let's save vmexit info into our vmm context
		{
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&vmm_context[current_processor].vmexit_qualification));
			vmm_context[current_processor].vmexit_reason = vmexit_reason.flags;
			__vmx_vmread(VMCS_GUEST_RIP, reinterpret_cast<size_t*>(&vmm_context[current_processor].vmexit_guest_rip));
			__vmx_vmread(VMCS_GUEST_RSP, reinterpret_cast<size_t*>(&vmm_context[current_processor].vmexit_guest_rsp));
			__vmx_vmread(VMCS_VMEXIT_INSTRUCTION_LENGTH, reinterpret_cast<size_t*>(&vmm_context[current_processor].vmexit_instruction_length));
			__vmx_vmread(VMCS_VMEXIT_INSTRUCTION_INFO, reinterpret_cast<size_t*>(&vmm_context[current_processor].vmexit_instruction_information));
		}


		switch (vmexit_reason.basic_exit_reason)
		{
		case VMX_EXIT_REASON_EXCEPTION_OR_NMI: {
			LOG("[*] exception or nmi\n");
		}
											 break;

		case VMX_EXIT_REASON_EXTERNAL_INTERRUPT: {
			LOG("[*] external interrupt\n");
		}
											   break;

		case VMX_EXIT_REASON_TRIPLE_FAULT: {
			LOG("[*] triple fault\n");
		}
										 break;

		case VMX_EXIT_REASON_INIT_SIGNAL: {
			LOG("[*] init signal\n");
		}
										break;

		case VMX_EXIT_REASON_STARTUP_IPI: {
			LOG("[*] startup ipi\n");
		}
										break;

		case VMX_EXIT_REASON_IO_SMI: {
			LOG("[*] io smi\n");
		}
								   break;

		case VMX_EXIT_REASON_SMI: {
			LOG("[*] smi\n");
		}
								break;

		case VMX_EXIT_REASON_INTERRUPT_WINDOW: {
			LOG("[*] interrupt window\n");
		}
											 break;

		case VMX_EXIT_REASON_NMI_WINDOW: {
			LOG("[*] nmi window\n");
		}
									   break;

		case VMX_EXIT_REASON_TASK_SWITCH: {
			LOG("[*] task switch\n");
			vmx_exit_qualification_task_switch exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
		}
										break;

		case VMX_EXIT_REASON_EXECUTE_CPUID: {
			LOG("[*] execute cpuid\n");
			int cpuInfo[4] = { 0 };
			__cpuidex(reinterpret_cast<int*>(&cpuInfo), static_cast<int>(guest_regs->rax), static_cast<int>(guest_regs->rcx));
		}
										  break;

		case VMX_EXIT_REASON_EXECUTE_GETSEC: {
			LOG("[*] execute getsec\n");
		}
										   break;

		case VMX_EXIT_REASON_EXECUTE_HLT: {
			LOG("[*] execute hlt\n");

			//
			// Terminate HyperVisor
			//
			//HostTerminateHypervisor();
		}
										break;

		case VMX_EXIT_REASON_EXECUTE_INVD: {
			LOG("[*] execute invd\n");
		}
										 break;

		case VMX_EXIT_REASON_EXECUTE_INVLPG: {
			LOG("[*] execute invlpg\n");
		}
										   break;

		case VMX_EXIT_REASON_EXECUTE_RDPMC: {
			LOG("[*] execute rdpmc\n");
		}
										  break;

		case VMX_EXIT_REASON_EXECUTE_RDTSC: {
			LOG("[*] execute rdtsc\n");
		}
										  break;

		case VMX_EXIT_REASON_EXECUTE_RSM_IN_SMM: {
			LOG("[*] execute rsm in smm\n");
		}
											   break;

		case VMX_EXIT_REASON_EXECUTE_VMCALL: {
			LOG("[*] execute vmcall\n");
		}
										   break;

		case VMX_EXIT_REASON_EXECUTE_VMCLEAR: {
			LOG("[*] execute vmclear\n");
		}
											break;

		case VMX_EXIT_REASON_EXECUTE_VMLAUNCH: {
			LOG("[*] execute vmlaunch\n");
		}
											 break;

		case VMX_EXIT_REASON_EXECUTE_VMPTRLD: {
			LOG("[*] execute vmptrld\n");
			vmx_vmexit_instruction_info_vmx_and_xsaves exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
		}
											break;

		case VMX_EXIT_REASON_EXECUTE_VMPTRST: {
			LOG("[*] execute vmptrst\n");
			vmx_vmexit_instruction_info_vmx_and_xsaves exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
		}
											break;

		case VMX_EXIT_REASON_EXECUTE_VMREAD: {
			LOG("[*] execute vmread\n");
			vmx_vmexit_instruction_info_vmread_vmwrite exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
		}
										   break;

		case VMX_EXIT_REASON_EXECUTE_VMRESUME: {
			LOG("[*] execute vmresume\n");
		}
											 break;

		case VMX_EXIT_REASON_EXECUTE_VMWRITE: {
			LOG("[*] execute vmwrite\n");
			vmx_vmexit_instruction_info_vmread_vmwrite exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
		}
											break;

		case VMX_EXIT_REASON_EXECUTE_VMXOFF: {
			LOG("[*] execute vmxoff\n");
		}
										   break;

		case VMX_EXIT_REASON_EXECUTE_VMXON: {
			LOG("[*] execute vmxon\n");
			vmx_vmexit_instruction_info_vmx_and_xsaves exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
		}
										  break;

		case VMX_EXIT_REASON_MOV_CR: {
			LOG("[*] mov cr\n");

			//
			// Check whether it was a mov to or mov from CR
			//
			vmx_exit_qualification_mov_cr exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
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
					__vmx_vmread(VMCS_GUEST_CR0, &guest_regs->rcx);
				}
					  break;

				case 3: {		// CR3
					__vmx_vmread(VMCS_GUEST_CR3, &guest_regs->rcx);
				}
					  break;

				case 4: {		// CR4
					__vmx_vmread(VMCS_GUEST_CR4, &guest_regs->rcx);
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
			LOG("[*] mov dr\n");
			vmx_exit_qualification_mov_dr exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
		}
								   break;

		case VMX_EXIT_REASON_EXECUTE_IO_INSTRUCTION: {
			LOG("[*] execute io\n");
			vmx_exit_qualification_io_instruction exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
		}
												   break;

		case VMX_EXIT_REASON_EXECUTE_RDMSR: {
			LOG("[*] execute rdmsr\n");
			if ((guest_regs->rcx <= 0x00001FFF) ||
				((guest_regs->rcx >= 0xC0000000) && (guest_regs->rcx <= 0xC0001FFF))) {

				LARGE_INTEGER msr;
				msr.QuadPart = __readmsr(static_cast<ulong>(guest_regs->rcx));
				guest_regs->rdx = msr.HighPart;
				guest_regs->rax = msr.LowPart;
			}
		}
										  break;

		case VMX_EXIT_REASON_EXECUTE_WRMSR: {
			LOG("[*] execute wrmsr\n");
			if ((guest_regs->rcx <= 0x00001FFF) ||
				((guest_regs->rcx >= 0xC0000000) && (guest_regs->rcx <= 0xC0001FFF))) {

				LARGE_INTEGER msr;
				msr.LowPart = static_cast<ulong>(guest_regs->rax);
				msr.HighPart = static_cast<ulong>(guest_regs->rdx);
				__writemsr(static_cast<ulong>(guest_regs->rcx), msr.QuadPart);
			}

		}
										  break;

		case VMX_EXIT_REASON_ERROR_INVALID_GUEST_STATE: {
			LOG("[*] invalid guest state\n");
		}
													  break;

		case VMX_EXIT_REASON_ERROR_MSR_LOAD: {
			LOG("[*] error msr load\n");
		}
										   break;

		case VMX_EXIT_REASON_EXECUTE_MWAIT: {
			LOG("[*] execute mwait\n");
		}
										  break;

		case VMX_EXIT_REASON_MONITOR_TRAP_FLAG: {
			LOG("[*] monitor trap flag\n");
		}
											  break;

		case VMX_EXIT_REASON_EXECUTE_MONITOR: {
			LOG("[*] execute monitor\n");
		}
											break;

		case VMX_EXIT_REASON_EXECUTE_PAUSE: {
			LOG("[*] execute pause\n");
		}
										  break;

		case VMX_EXIT_REASON_ERROR_MACHINE_CHECK: {
			LOG("[*] error machine check\n");
		}
												break;

		case VMX_EXIT_REASON_TPR_BELOW_THRESHOLD: {
			LOG("[*] below threshold\n");
		}
												break;

		case VMX_EXIT_REASON_APIC_ACCESS: {
			LOG("[*] apic access\n");
			vmx_exit_qualification_apic_access exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
		}
										break;

		case VMX_EXIT_REASON_VIRTUALIZED_EOI: {
			LOG("[*] virtualized eoi\n");
		}
											break;

		case VMX_EXIT_REASON_GDTR_IDTR_ACCESS: {
			LOG("[*] gdtr idtr access\n");
			vmx_vmexit_instruction_info_gdtr_idtr_access exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
		}
											 break;

		case VMX_EXIT_REASON_LDTR_TR_ACCESS: {
			LOG("[*] ldtr tr access\n");
			vmx_vmexit_instruction_info_ldtr_tr_access exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
		}
										   break;

		case VMX_EXIT_REASON_EPT_VIOLATION: {
			LOG("[*] ept violation\n");

			vmx_exit_qualification_ept_violation exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));

			uint64_t phys_addr;
			__vmx_vmread(VMCS_GUEST_PHYSICAL_ADDRESS, &phys_addr);
			//phys_addr = PAGE_ALIGN((PVOID)phys_addr);


			uint64_t linear_addr;
			__vmx_vmread(VMCS_EXIT_GUEST_LINEAR_ADDRESS, &linear_addr);

			if (exitQualification.ept_executable || exitQualification.ept_readable || exitQualification.ept_writeable) {
				__debugbreak();
				LOG("Error: VA = %llx, PA = %llx", linear_addr, phys_addr);
				return;
			}

			//HandleEptViolation(phys_addr, linear_addr);
		}
										  break;

		case VMX_EXIT_REASON_EPT_MISCONFIGURATION: {
			LOG("[*] EPT Misconfiguration\n");
			LOG_ERROR();
			__debugbreak();

			// Failure setting EPT
			// Bugcheck and restart system
			KeBugCheck(PFN_LIST_CORRUPT);	// Is this bug code even correct??
		}
												 break;

		case VMX_EXIT_REASON_EXECUTE_INVEPT: {
			LOG("[*] invept\n");
			vmx_vmexit_instruction_info_invalidate exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
			__debugbreak();
		}
										   break;

		case VMX_EXIT_REASON_EXECUTE_RDTSCP: {
			LOG("[*] rdtscp\n");
		}
										   break;

		case VMX_EXIT_REASON_VMX_PREEMPTION_TIMER_EXPIRED: {
			LOG("[*] timer expired\n");
		}
														 break;

		case VMX_EXIT_REASON_EXECUTE_INVVPID: {
			LOG("[*] invvpid\n");
			vmx_vmexit_instruction_info_invalidate exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
		}
											break;

		case VMX_EXIT_REASON_EXECUTE_WBINVD: {
			LOG("[*] wbinvd\n");
		}
										   break;

		case VMX_EXIT_REASON_EXECUTE_XSETBV: {
			LOG("[*] xsetvb\n");
		}
										   break;

		case VMX_EXIT_REASON_APIC_WRITE: {
			LOG("[*] apic write\n");
		}
									   break;

		case VMX_EXIT_REASON_EXECUTE_RDRAND: {
			LOG("[*] rdrand\n");
			vmx_vmexit_instruction_info_rdrand_rdseed exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
		}
										   break;

		case VMX_EXIT_REASON_EXECUTE_INVPCID: {
			LOG("[*] invpcid\n");
			vmx_vmexit_instruction_info_invalidate exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
		}
											break;

		case VMX_EXIT_REASON_EXECUTE_VMFUNC: {
			LOG("[*] vmfunc\n");
		}
										   break;

		case VMX_EXIT_REASON_EXECUTE_ENCLS: {
			LOG("[*] execute encls\n");
		}
										  break;

		case VMX_EXIT_REASON_EXECUTE_RDSEED: {
			LOG("[*] execute rdseed\n");
		}
										   break;

		case VMX_EXIT_REASON_PAGE_MODIFICATION_LOG_FULL: {
			LOG("[*] modification log full\n");
		}
													   break;

		case VMX_EXIT_REASON_EXECUTE_XSAVES: {
			LOG("[*] execute xsaves\n");
			vmx_vmexit_instruction_info_vmx_and_xsaves exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
		}
										   break;

		case VMX_EXIT_REASON_EXECUTE_XRSTORS: {
			LOG("[*] execute xstors\n");
			vmx_vmexit_instruction_info_vmx_and_xsaves exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
		}
											break;

		default:
			break;
		}

		return;
	}
}