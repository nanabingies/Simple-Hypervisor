#include "stdafx.h"

namespace vmexit {
	auto vmexit_handler(void* _guest_registers) -> short {
		auto guest_regs = reinterpret_cast<guest_registers*>(_guest_registers);
		if (!guest_regs)	return VM_ERROR_ERR_INFO_ERR;

		vmx_vmexit_reason vmexit_reason;
		__vmx_vmread(VMCS_EXIT_REASON, reinterpret_cast<size_t*>(&vmexit_reason));

		switch (vmexit_reason.basic_exit_reason)
		{
		case VMX_EXIT_REASON_EXCEPTION_OR_NMI: {
			//LOG("[*][%ws] exception or nmi\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXTERNAL_INTERRUPT: {
			//LOG("[*][%ws] external interrupt\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_TRIPLE_FAULT: {
			//LOG("[*][%ws] triple fault\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_INIT_SIGNAL: {
			//LOG("[*][%ws] init signal\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_STARTUP_IPI: {
			//LOG("[*][%ws] startup ipi\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_IO_SMI: {
			//LOG("[*][%ws] io smi\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_SMI: {
			//LOG("[*][%ws] smi\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_INTERRUPT_WINDOW: {
			//LOG("[*][%ws] interrupt window\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_NMI_WINDOW: {
			//LOG("[*][%ws] nmi window\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_TASK_SWITCH: {
			//LOG("[*][%ws] task switch\n", __FUNCTIONW__);
			vmx_exit_qualification_task_switch exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_CPUID: {
			//LOG("[*][%ws] execute cpuid\n", __FUNCTIONW__);
			int cpuInfo[4] = { 0 };
			__cpuidex(reinterpret_cast<int*>(&cpuInfo), static_cast<int>(guest_regs->rax), static_cast<int>(guest_regs->rcx));
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_GETSEC: {
			//LOG("[*][%ws] execute getsec\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_HLT: {
			//LOG("[*][%ws] execute hlt\n", __FUNCTIONW__);

			//
			// Terminate HyperVisor
			//
			//HostTerminateHypervisor();
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_INVD: {
			//LOG("[*][%ws] execute invd\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_INVLPG: {
			//LOG("[*][%ws] execute invlpg\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_RDPMC: {
			//LOG("[*][%ws] execute rdpmc\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_RDTSC: {
			//LOG("[*][%ws] execute rdtsc\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_RSM_IN_SMM: {
			//LOG("[*][%ws] execute rsm in smm\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_VMCALL: {
			//LOG("[*][%ws] execute vmcall\n", __FUNCTIONW__);
			// TODO: handle vmcall
			// We might use it to execute vmxoff and exit from hypervisor
			guest_regs->rax = vmx::vmx_handle_vmcall(guest_regs->rcx, guest_regs->rdx, guest_regs->r8, guest_regs->r9);
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_VMCLEAR: {
			//LOG("[*][%ws] execute vmclear\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_VMLAUNCH: {
			//LOG("[*][%ws] execute vmlaunch\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_VMPTRLD: {
			//LOG("[*][%ws] execute vmptrld\n", __FUNCTIONW__);
			vmx_vmexit_instruction_info_vmx_and_xsaves exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_VMPTRST: {
			//LOG("[*][%ws] execute vmptrst\n", __FUNCTIONW__);
			vmx_vmexit_instruction_info_vmx_and_xsaves exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_VMREAD: {
			//LOG("[*][%ws] execute vmread\n", __FUNCTIONW__);
			vmx_vmexit_instruction_info_vmread_vmwrite exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_VMRESUME: {
			//LOG("[*][%ws] execute vmresume\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_VMWRITE: {
			//LOG("[*][%ws] execute vmwrite\n", __FUNCTIONW__);
			vmx_vmexit_instruction_info_vmread_vmwrite exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_VMXOFF: {
			//LOG("[*][%ws] execute vmxoff\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_VMXON: {
			//LOG("[*][%ws] execute vmxon\n", __FUNCTIONW__);
			vmx_vmexit_instruction_info_vmx_and_xsaves exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
			goto move_rip;
		}

		case VMX_EXIT_REASON_MOV_CR: {
			//LOG("[*][%ws] mov cr\n", __FUNCTIONW__);

			// Check whether it was a mov to or mov from CR
			vmx_exit_qualification_mov_cr exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
			switch (exitQualification.access_type) {
			case 0: {	// MOV to CR
				switch (exitQualification.control_register) {
				case 0: {	// CR0
					__vmx_vmwrite(VMCS_GUEST_CR0, __readcr0());
					__vmx_vmwrite(VMCS_CTRL_CR0_READ_SHADOW, __readcr0());
					goto move_rip;
				}

				case 3: {	// CR3
					__vmx_vmwrite(VMCS_GUEST_CR3, __readcr3());
					goto move_rip;
				}

				case 4: {	// CR4
					__vmx_vmwrite(VMCS_GUEST_CR4, __readcr4());
					__vmx_vmwrite(VMCS_CTRL_CR4_READ_SHADOW, __readcr4());
					goto move_rip;
				}

				default:
					goto exit;
				}
			}
				  break;

			case 1: {	// MOV from CR
				switch (exitQualification.control_register) {
				case 0: {		// CR0
					__vmx_vmread(VMCS_GUEST_CR0, &guest_regs->rcx);
					goto move_rip;
				}

				case 3: {		// CR3
					__vmx_vmread(VMCS_GUEST_CR3, &guest_regs->rcx);
					goto move_rip;
				}

				case 4: {		// CR4
					__vmx_vmread(VMCS_GUEST_CR4, &guest_regs->rcx);
					goto move_rip;
				}
				}
			}
				  goto move_rip;

			case 2: {	// 2 = CLTS
				goto move_rip;
			}

			case 3: {	// 3 = LMSW
				goto move_rip;
			}

			default:
				goto exit;
			}
			goto move_rip;
		}

		case VMX_EXIT_REASON_MOV_DR: {
			//LOG("[*][%ws] mov dr\n", __FUNCTIONW__);
			vmx_exit_qualification_mov_dr exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_IO_INSTRUCTION: {
			//LOG("[*][%ws] execute io\n", __FUNCTIONW__);
			vmx_exit_qualification_io_instruction exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_RDMSR: {
			//LOG("[*][%ws] execute rdmsr\n", __FUNCTIONW__);
			if ((guest_regs->rcx <= 0x00001FFF) ||
				((guest_regs->rcx >= 0xC0000000) && (guest_regs->rcx <= 0xC0001FFF))) {

				LARGE_INTEGER msr;
				msr.QuadPart = __readmsr(static_cast<ulong>(guest_regs->rcx));
				guest_regs->rdx = msr.HighPart;
				guest_regs->rax = msr.LowPart;
			}
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_WRMSR: {
			//LOG("[*][%ws] execute wrmsr\n", __FUNCTIONW__);
			if ((guest_regs->rcx <= 0x00001FFF) ||
				((guest_regs->rcx >= 0xC0000000) && (guest_regs->rcx <= 0xC0001FFF))) {

				LARGE_INTEGER msr;
				msr.LowPart = static_cast<ulong>(guest_regs->rax);
				msr.HighPart = static_cast<ulong>(guest_regs->rdx);
				__writemsr(static_cast<ulong>(guest_regs->rcx), msr.QuadPart);
			}

			goto move_rip;
		}

		case VMX_EXIT_REASON_ERROR_INVALID_GUEST_STATE: {
			//LOG("[*][%ws] invalid guest state\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_ERROR_MSR_LOAD: {
			//LOG("[*][%ws] error msr load\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_MWAIT: {
			//LOG("[*][%ws] execute mwait\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_MONITOR_TRAP_FLAG: {
			//LOG("[*][%ws] monitor trap flag\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_MONITOR: {
			//LOG("[*][%ws] execute monitor\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_PAUSE: {
			//LOG("[*][%ws] execute pause\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_ERROR_MACHINE_CHECK: {
			//LOG("[*][%ws] error machine check\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_TPR_BELOW_THRESHOLD: {
			//LOG("[*][%ws] below threshold\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_APIC_ACCESS: {
			LOG("[*][%ws] apic access\n", __FUNCTIONW__);
			vmx_exit_qualification_apic_access exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
			goto move_rip;
		}

		case VMX_EXIT_REASON_VIRTUALIZED_EOI: {
			//LOG("[*][%ws] virtualized eoi\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_GDTR_IDTR_ACCESS: {
			//LOG("[*][%ws] gdtr idtr access\n", __FUNCTIONW__);
			vmx_vmexit_instruction_info_gdtr_idtr_access exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
			goto move_rip;
		}

		case VMX_EXIT_REASON_LDTR_TR_ACCESS: {
			//LOG("[*][%ws] ldtr tr access\n", __FUNCTIONW__);
			vmx_vmexit_instruction_info_ldtr_tr_access exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
			goto move_rip;
		}

		case VMX_EXIT_REASON_EPT_VIOLATION: {
			using ept::handle_ept_violation;
			//LOG("[*][%ws] ept violation\n", __FUNCTIONW__);

			vmx_exit_qualification_ept_violation exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));

			uint64_t phys_addr;
			__vmx_vmread(VMCS_GUEST_PHYSICAL_ADDRESS, &phys_addr);
			//phys_addr = PAGE_ALIGN((PVOID)phys_addr);


			uint64_t linear_addr;
			__vmx_vmread(VMCS_EXIT_GUEST_LINEAR_ADDRESS, &linear_addr);

			if (exitQualification.ept_executable || exitQualification.ept_readable || exitQualification.ept_writeable) {
				// These caused an EPT Violation
				__debugbreak();
				LOG("Error: VA = %llx, PA = %llx", linear_addr, phys_addr);
				return VM_ERROR_ERR_INFO_ERR;
			}

			if (!handle_ept_violation(phys_addr, linear_addr)) {
				LOG("[!][%ws] Error handling apt violation\n", __FUNCTIONW__);
			}
			goto move_rip;
		}

		case VMX_EXIT_REASON_EPT_MISCONFIGURATION: {
			//LOG("[*][%ws] EPT Misconfiguration\n", __FUNCTIONW__);
			//LOG_ERROR();
			__debugbreak();

			// Failure setting EPT
			// Bugcheck and restart system
			KeBugCheck(PFN_LIST_CORRUPT);	// Is this bug code even correct??
		}

		case VMX_EXIT_REASON_EXECUTE_INVEPT: {
			//LOG("[*][%ws] invept\n", __FUNCTIONW__);
			vmx_vmexit_instruction_info_invalidate exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
			__debugbreak();
			goto exit;
		}

		case VMX_EXIT_REASON_EXECUTE_RDTSCP: {
			//LOG("[*][%ws] rdtscp\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_VMX_PREEMPTION_TIMER_EXPIRED: {
			//LOG("[*][%ws] timer expired\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_INVVPID: {
			//LOG("[*][%ws] invvpid\n", __FUNCTIONW__);
			vmx_vmexit_instruction_info_invalidate exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_WBINVD: {
			//LOG("[*][%ws] wbinvd\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_XSETBV: {
			//LOG("[*][%ws] xsetvb\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_APIC_WRITE: {
			//LOG("[*][%ws] apic write\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_RDRAND: {
			//LOG("[*][%ws] rdrand\n", __FUNCTIONW__);
			vmx_vmexit_instruction_info_rdrand_rdseed exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_INVPCID: {
			//LOG("[*][%ws] invpcid\n", __FUNCTIONW__);
			vmx_vmexit_instruction_info_invalidate exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_VMFUNC: {
			//LOG("[*][%ws] vmfunc\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_ENCLS: {
			//LOG("[*][%ws] execute encls\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_RDSEED: {
			//LOG("[*][%ws] execute rdseed\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_PAGE_MODIFICATION_LOG_FULL: {
			//LOG("[*][%ws] modification log full\n", __FUNCTIONW__);
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_XSAVES: {
			//LOG("[*][%ws] execute xsaves\n", __FUNCTIONW__);
			vmx_vmexit_instruction_info_vmx_and_xsaves exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
			goto move_rip;
		}

		case VMX_EXIT_REASON_EXECUTE_XRSTORS: {
			//LOG("[*][%ws] execute xstors\n", __FUNCTIONW__);
			vmx_vmexit_instruction_info_vmx_and_xsaves exitQualification;
			__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
			goto move_rip;
		}

		default:
			break;
		}

	move_rip:
		size_t rip, inst_len;
		__vmx_vmread(VMCS_GUEST_RIP, &rip);
		__vmx_vmread(VMCS_VMEXIT_INSTRUCTION_LENGTH, &inst_len);

		rip += inst_len;
		__vmx_vmwrite(VMCS_GUEST_RIP, rip);
		return VM_ERROR_OK;
	
	exit:
		return VM_ERROR_ERR_INFO_OK;
	}
}