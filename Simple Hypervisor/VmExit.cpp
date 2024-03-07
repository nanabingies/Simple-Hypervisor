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
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*)&exitQualification);
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
			VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*)&exitQualification);
		}
											break;

		case VMX_EXIT_REASON_EXECUTE_VMPTRST: {
			LOG("[*] execute vmptrst\n");
			VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*)&exitQualification);
		}
											break;

		case VMX_EXIT_REASON_EXECUTE_VMREAD: {
			LOG("[*] execute vmread\n");
			VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*)&exitQualification);
		}
										   break;

		case VMX_EXIT_REASON_EXECUTE_VMRESUME: {
			LOG("[*] execute vmresume\n");
		}
											 break;

		case VMX_EXIT_REASON_EXECUTE_VMWRITE: {
			LOG("[*] execute vmwrite\n");
			VMX_VMEXIT_INSTRUCTION_INFO_VMREAD_VMWRITE exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*)&exitQualification);
		}
											break;

		case VMX_EXIT_REASON_EXECUTE_VMXOFF: {
			LOG("[*] execute vmxoff\n");
		}
										   break;

		case VMX_EXIT_REASON_EXECUTE_VMXON: {
			LOG("[*] execute vmxon\n");
			VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*)&exitQualification);
		}
										  break;

		case VMX_EXIT_REASON_MOV_CR: {
			LOG("[*] mov cr\n");

			//
			// Check whether it was a mov to or mov from CR
			//
			VMX_EXIT_QUALIFICATION_MOV_CR exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*)&exitQualification);
			switch (exitQualification.AccessType) {
			case 0: {	// MOV to CR
				switch (exitQualification.ControlRegister) {
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
				switch (exitQualification.ControlRegister) {
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
			VMX_EXIT_QUALIFICATION_MOV_DR exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*)&exitQualification);
		}
								   break;

		case VMX_EXIT_REASON_EXECUTE_IO_INSTRUCTION: {
			DbgPrint("[*] execute io\n");
			VMX_EXIT_QUALIFICATION_IO_INSTRUCTION exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*)&exitQualification);
		}
												   break;

		case VMX_EXIT_REASON_EXECUTE_RDMSR: {
			DbgPrint("[*] execute rdmsr\n");
			if ((guestRegisters->RCX <= 0x00001FFF) ||
				((guestRegisters->RCX >= 0xC0000000) && (guestRegisters->RCX <= 0xC0001FFF))) {

				LARGE_INTEGER msr;
				msr.QuadPart = __readmsr((ULONG)guestRegisters->RCX);
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
				msr.LowPart = (ULONG)guestRegisters->RAX;
				msr.HighPart = (ULONG)guestRegisters->RDX;
				__writemsr((ULONG)guestRegisters->RCX, msr.QuadPart);
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
			VMX_EXIT_QUALIFICATION_APIC_ACCESS exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*)&exitQualification);
		}
										break;

		case VMX_EXIT_REASON_VIRTUALIZED_EOI: {
			DbgPrint("[*] virtualized eoi\n");
		}
											break;

		case VMX_EXIT_REASON_GDTR_IDTR_ACCESS: {
			DbgPrint("[*] gdtr idtr access\n");
			VMX_VMEXIT_INSTRUCTION_INFO_GDTR_IDTR_ACCESS exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*)&exitQualification);
		}
											 break;

		case VMX_EXIT_REASON_LDTR_TR_ACCESS: {
			DbgPrint("[*] ldtr tr access\n");
			VMX_VMEXIT_INSTRUCTION_INFO_LDTR_TR_ACCESS exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*)&exitQualification);
		}
										   break;

		case VMX_EXIT_REASON_EPT_VIOLATION: {
			DbgPrint("[*] ept violation\n");

			VMX_EXIT_QUALIFICATION_EPT_VIOLATION exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*)&exitQualification);

			UINT64 phys_addr;
			__vmx_vmread(VMCS_GUEST_PHYSICAL_ADDRESS, &phys_addr);
			//phys_addr = PAGE_ALIGN((PVOID)phys_addr);


			UINT64 linear_addr;
			__vmx_vmread(VMCS_EXIT_GUEST_LINEAR_ADDRESS, &linear_addr);

			if (exitQualification.EptExecutable || exitQualification.EptReadable || exitQualification.EptWriteable) {
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
			VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*)&exitQualification);
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
			VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*)&exitQualification);
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
			VMX_VMEXIT_INSTRUCTION_INFO_RDRAND_RDSEED exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*)&exitQualification);
		}
										   break;

		case VMX_EXIT_REASON_EXECUTE_INVPCID: {
			DbgPrint("[*] invpcid\n");
			VMX_VMEXIT_INSTRUCTION_INFO_INVALIDATE exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*)&exitQualification);
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
			VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*)&exitQualification);
		}
										   break;

		case VMX_EXIT_REASON_EXECUTE_XRSTORS: {
			DbgPrint("[*] execute xstors\n");
			VMX_VMEXIT_INSTRUCTION_INFO_VMX_AND_XSAVES exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, (size_t*)&exitQualification);
		}
											break;

		default:
			break;
		}

		return;
	}
}