#include "stdafx.h"

namespace vmexit {
	auto vmexit_handler(void* args) -> void {
		auto guest_regs = reinterpret_cast<guest_registers*>(args);
		if (!guest_regs)	return;

		vmx_vmexit_reason vmexit_reason;
		__vmx_vmread(VMCS_EXIT_REASON, reinterpret_cast<size_t*>(&vmexit_reason));

		__vcpu* vcpu = g_vmm_context->vcpu_table[KeGetCurrentProcessorNumberEx(NULL)];

		unsigned __int64 vmcs_guest_rsp = 0;
		__vmx_vmread(VMCS_GUEST_RSP, &vmcs_guest_rsp);

		{
			// Save vmexit info
			guest_regs->rsp = vmcs_guest_rsp;
			vcpu->vmexit_info.guest_registers = guest_regs;
		}

		//LOG("[*] exit reason : %llx\n", vmexit_reason.basic_exit_reason);
		//LOG("[*] guest rsp : %llx\n", vmcs_guest_rsp);
		//LOG("[>]=======================================================================[<]\n\n");

		//__debugbreak();

		switch (vmexit_reason.basic_exit_reason)
		{
		case VMX_EXIT_REASON_EXCEPTION_OR_NMI: {
			//LOG("[*][%ws] exception or nmi\n", __FUNCTIONW__);
		}
											 break;

		case VMX_EXIT_REASON_EXTERNAL_INTERRUPT: {
			//LOG("[*][%ws] external interrupt\n", __FUNCTIONW__);
		}
											   break;

		case VMX_EXIT_REASON_TRIPLE_FAULT: {
			//LOG("[*][%ws] triple fault\n", __FUNCTIONW__);
		}
										 break;

		case VMX_EXIT_REASON_INIT_SIGNAL: {
			//LOG("[*][%ws] init signal\n", __FUNCTIONW__);
		}
										break;

		case VMX_EXIT_REASON_STARTUP_IPI: {
			//LOG("[*][%ws] startup ipi\n", __FUNCTIONW__);
		}
										break;

		case VMX_EXIT_REASON_IO_SMI: {
			//LOG("[*][%ws] io smi\n", __FUNCTIONW__);
		}
								   break;

		case VMX_EXIT_REASON_SMI: {
			//LOG("[*][%ws] smi\n", __FUNCTIONW__);
		}
								break;

		case VMX_EXIT_REASON_INTERRUPT_WINDOW: {
			//LOG("[*][%ws] interrupt window\n", __FUNCTIONW__);
		}
											 break;

		case VMX_EXIT_REASON_NMI_WINDOW: {
			//LOG("[*][%ws] nmi window\n", __FUNCTIONW__);
		}
									   break;

		case VMX_EXIT_REASON_TASK_SWITCH: {
			//LOG("[*][%ws] task switch\n", __FUNCTIONW__);
			handle_task_switch(guest_regs);
		}
										break;

		case VMX_EXIT_REASON_EXECUTE_CPUID: {
			//LOG("[*][%ws] execute cpuid\n", __FUNCTIONW__);
			handle_execute_cpuid(guest_regs);
		}
										  break;

		case VMX_EXIT_REASON_EXECUTE_GETSEC: {
			//LOG("[*][%ws] execute getsec\n", __FUNCTIONW__);
		}
										   break;

		case VMX_EXIT_REASON_EXECUTE_HLT: {
			//LOG("[*][%ws] execute hlt\n", __FUNCTIONW__);

			//
			// Terminate HyperVisor
			//
			//HostTerminateHypervisor();
		}
										break;

		case VMX_EXIT_REASON_EXECUTE_INVD: {
			//LOG("[*][%ws] execute invd\n", __FUNCTIONW__);
		}
										 break;

		case VMX_EXIT_REASON_EXECUTE_INVLPG: {
			//LOG("[*][%ws] execute invlpg\n", __FUNCTIONW__);
		}
										   break;

		case VMX_EXIT_REASON_EXECUTE_RDPMC: {
			//LOG("[*][%ws] execute rdpmc\n", __FUNCTIONW__);
		}
										  break;

		case VMX_EXIT_REASON_EXECUTE_RDTSC: {
			//LOG("[*][%ws] execute rdtsc\n", __FUNCTIONW__);
		}
										  break;

		case VMX_EXIT_REASON_EXECUTE_RSM_IN_SMM: {
			//LOG("[*][%ws] execute rsm in smm\n", __FUNCTIONW__);
		}
											   break;

		case VMX_EXIT_REASON_EXECUTE_VMCALL: {
			//LOG("[*][%ws] execute vmcall\n", __FUNCTIONW__);
			handle_execute_vmcall(guest_regs);
		}
										   break;

		case VMX_EXIT_REASON_EXECUTE_VMCLEAR: {
			//LOG("[*][%ws] execute vmclear\n", __FUNCTIONW__);
		}
											break;

		case VMX_EXIT_REASON_EXECUTE_VMLAUNCH: {
			//LOG("[*][%ws] execute vmlaunch\n", __FUNCTIONW__);
		}
											 break;

		case VMX_EXIT_REASON_EXECUTE_VMPTRLD: {
			//LOG("[*][%ws] execute vmptrld\n", __FUNCTIONW__);
			handle_execute_vmptrld(guest_regs);
		}
											break;

		case VMX_EXIT_REASON_EXECUTE_VMPTRST: {
			//LOG("[*][%ws] execute vmptrst\n", __FUNCTIONW__);
			handle_execute_vmptrst(guest_regs);
		}
											break;

		case VMX_EXIT_REASON_EXECUTE_VMREAD: {
			//LOG("[*][%ws] execute vmread\n", __FUNCTIONW__);
			handle_execute_vmread(guest_regs);
		}
										   break;

		case VMX_EXIT_REASON_EXECUTE_VMRESUME: {
			//LOG("[*][%ws] execute vmresume\n", __FUNCTIONW__);
		}
											 break;

		case VMX_EXIT_REASON_EXECUTE_VMWRITE: {
			//LOG("[*][%ws] execute vmwrite\n", __FUNCTIONW__);
			handle_execute_vmwrite(guest_regs);
		}
											break;

		case VMX_EXIT_REASON_EXECUTE_VMXOFF: {
			//LOG("[*][%ws] execute vmxoff\n", __FUNCTIONW__);
		}
										   break;

		case VMX_EXIT_REASON_EXECUTE_VMXON: {
			//LOG("[*][%ws] execute vmxon\n", __FUNCTIONW__);
			handle_execute_vmxon(guest_regs);
		}
										  break;

		case VMX_EXIT_REASON_MOV_CR: {
			//LOG("[*][%ws] mov cr\n", __FUNCTIONW__);
			handle_mov_cr(guest_regs);
		}
								   break;

		case VMX_EXIT_REASON_MOV_DR: {
			//LOG("[*][%ws] mov dr\n", __FUNCTIONW__);
			handle_mov_dr(guest_regs);
		}
								   break;

		case VMX_EXIT_REASON_EXECUTE_IO_INSTRUCTION: {
			//LOG("[*][%ws] execute io\n", __FUNCTIONW__);
			handle_execute_io_instruction(guest_regs);
		}
												   break;

		case VMX_EXIT_REASON_EXECUTE_RDMSR: {
			//LOG("[*][%ws] execute rdmsr\n", __FUNCTIONW__);
			handle_rdmsr(guest_regs);
		}
										  break;

		case VMX_EXIT_REASON_EXECUTE_WRMSR: {
			handle_wrmsr(guest_regs);
		}
										  break;

		case VMX_EXIT_REASON_ERROR_INVALID_GUEST_STATE: {
			//LOG("[*][%ws] invalid guest state\n", __FUNCTIONW__);
		}
													  break;

		case VMX_EXIT_REASON_ERROR_MSR_LOAD: {
			//LOG("[*][%ws] error msr load\n", __FUNCTIONW__);
		}
										   break;

		case VMX_EXIT_REASON_EXECUTE_MWAIT: {
			//LOG("[*][%ws] execute mwait\n", __FUNCTIONW__);
		}
										  break;

		case VMX_EXIT_REASON_MONITOR_TRAP_FLAG: {
			//LOG("[*][%ws] monitor trap flag\n", __FUNCTIONW__);
		}
											  break;

		case VMX_EXIT_REASON_EXECUTE_MONITOR: {
			//LOG("[*][%ws] execute monitor\n", __FUNCTIONW__);
		}
											break;

		case VMX_EXIT_REASON_EXECUTE_PAUSE: {
			//LOG("[*][%ws] execute pause\n", __FUNCTIONW__);
		}
										  break;

		case VMX_EXIT_REASON_ERROR_MACHINE_CHECK: {
			//LOG("[*][%ws] error machine check\n", __FUNCTIONW__);
		}
												break;

		case VMX_EXIT_REASON_TPR_BELOW_THRESHOLD: {
			//LOG("[*][%ws] below threshold\n", __FUNCTIONW__);
		}
												break;

		case VMX_EXIT_REASON_APIC_ACCESS: {
			//LOG("[*][%ws] apic access\n", __FUNCTIONW__);
			handle_apic_access(guest_regs);
		}
										break;

		case VMX_EXIT_REASON_VIRTUALIZED_EOI: {
			//LOG("[*][%ws] virtualized eoi\n", __FUNCTIONW__);
		}
											break;

		case VMX_EXIT_REASON_GDTR_IDTR_ACCESS: {
			//LOG("[*][%ws] gdtr idtr access\n", __FUNCTIONW__);
			handle_gdtr_idtr_access(guest_regs);
		}
											 break;

		case VMX_EXIT_REASON_LDTR_TR_ACCESS: {
			//LOG("[*][%ws] ldtr tr access\n", __FUNCTIONW__);
			handle_ldtr_tr_access(guest_regs);
		}
										   break;

		case VMX_EXIT_REASON_EPT_VIOLATION: {
			//LOG("[*][%ws] ept violation\n", __FUNCTIONW__);
			handle_ept_violation(guest_regs);
		}
										  break;

		case VMX_EXIT_REASON_EPT_MISCONFIGURATION: {
			//LOG("[*][%ws] EPT Misconfiguration\n", __FUNCTIONW__);
			handle_ept_misconfiguration(guest_regs);
		}
												 break;

		case VMX_EXIT_REASON_EXECUTE_INVEPT: {
			//LOG("[*][%ws] invept\n", __FUNCTIONW__);
			handle_execute_invept(guest_regs);
		}
										   break;

		case VMX_EXIT_REASON_EXECUTE_RDTSCP: {
			//LOG("[*][%ws] rdtscp\n", __FUNCTIONW__);
		}
										   break;

		case VMX_EXIT_REASON_VMX_PREEMPTION_TIMER_EXPIRED: {
			//LOG("[*][%ws] timer expired\n", __FUNCTIONW__);
		}
														 break;

		case VMX_EXIT_REASON_EXECUTE_INVVPID: {
			//LOG("[*][%ws] invvpid\n", __FUNCTIONW__);
			handle_execute_invvpid(guest_regs);
		}
											break;

		case VMX_EXIT_REASON_EXECUTE_WBINVD: {
			//LOG("[*][%ws] wbinvd\n", __FUNCTIONW__);
		}
										   break;

		case VMX_EXIT_REASON_EXECUTE_XSETBV: {
			//LOG("[*][%ws] xsetvb\n", __FUNCTIONW__);
		}
										   break;

		case VMX_EXIT_REASON_APIC_WRITE: {
			//LOG("[*][%ws] apic write\n", __FUNCTIONW__);
		}
									   break;

		case VMX_EXIT_REASON_EXECUTE_RDRAND: {
			//LOG("[*][%ws] rdrand\n", __FUNCTIONW__);
			handle_execute_rdrand(guest_regs);
		}
										   break;

		case VMX_EXIT_REASON_EXECUTE_INVPCID: {
			//LOG("[*][%ws] invpcid\n", __FUNCTIONW__);
			handle_execute_invpcid(guest_regs);
		}
											break;

		case VMX_EXIT_REASON_EXECUTE_VMFUNC: {
			//LOG("[*][%ws] vmfunc\n", __FUNCTIONW__);
		}
										   break;

		case VMX_EXIT_REASON_EXECUTE_ENCLS: {
			//LOG("[*][%ws] execute encls\n", __FUNCTIONW__);
		}
										  break;

		case VMX_EXIT_REASON_EXECUTE_RDSEED: {
			//LOG("[*][%ws] execute rdseed\n", __FUNCTIONW__);
		}
										   break;

		case VMX_EXIT_REASON_PAGE_MODIFICATION_LOG_FULL: {
			//LOG("[*][%ws] modification log full\n", __FUNCTIONW__);
		}
													   break;

		case VMX_EXIT_REASON_EXECUTE_XSAVES: {
			//LOG("[*][%ws] execute xsaves\n", __FUNCTIONW__);
			handle_execute_xsaves(guest_regs);
		}
										   break;

		case VMX_EXIT_REASON_EXECUTE_XRSTORS: {
			//LOG("[*][%ws] execute xstors\n", __FUNCTIONW__);
			handle_execute_xrstors(guest_regs);
		}
											break;

		default:
			break;
		}

		size_t vmcs_guest_rip, vmcs_guest_inst_len;
		__vmx_vmread(VMCS_GUEST_RIP, &vmcs_guest_rip);
		__vmx_vmread(VMCS_VMEXIT_INSTRUCTION_LENGTH, &vmcs_guest_inst_len);

		vmcs_guest_rip += vmcs_guest_inst_len;
		__vmx_vmwrite(VMCS_GUEST_RIP, vmcs_guest_rip);

		return;
	}

	auto handle_wrmsr(void* args) -> void {
		NT_ASSERTMSG("ARGS == NULL", args != nullptr);

		auto guest_regs = reinterpret_cast<guest_registers*>(args);
		if ((guest_regs->rcx <= 0x00001FFF) ||
			((guest_regs->rcx >= 0xC0000000) && (guest_regs->rcx <= 0xC0001FFF))) {

			LARGE_INTEGER msr;
			msr.LowPart = static_cast<ULONG>(guest_regs->rax);
			msr.HighPart = static_cast<ULONG>(guest_regs->rdx);
			__writemsr(static_cast<ULONG>(guest_regs->rcx), msr.QuadPart);
		}
	}

	auto handle_rdmsr(void* args) -> void {
		NT_ASSERTMSG("ARGS == NULL", args != nullptr);

		auto guest_regs = reinterpret_cast<guest_registers*>(args);
		if ((guest_regs->rcx <= 0x00001FFF) ||
			((guest_regs->rcx >= 0xC0000000) && (guest_regs->rcx <= 0xC0001FFF))) {

			LARGE_INTEGER msr;
			msr.QuadPart = __readmsr(static_cast<ulong>(guest_regs->rcx));
			guest_regs->rdx = msr.HighPart;
			guest_regs->rax = msr.LowPart;
		}
	}

	auto handle_task_switch(void* args) -> void {
		UNREFERENCED_PARAMETER(args);
		NT_ASSERTMSG("ARGS == NULL", args != nullptr);
		// TODO:
		vmx_exit_qualification_task_switch exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}

	auto handle_execute_cpuid(void* args) -> void {
		NT_ASSERTMSG("ARGS == NULL", args != nullptr);
		auto guest_regs = reinterpret_cast<guest_registers*>(args);
		int cpuInfo[4] = { 0 };
		__cpuidex(reinterpret_cast<int*>(&cpuInfo), static_cast<int>(guest_regs->rax), static_cast<int>(guest_regs->rcx));
	}

	auto handle_execute_vmcall(void* args) -> void {
		NT_ASSERTMSG("ARGS == NULL", args != nullptr);
		// TODO: handle vmcall
		// We might use it to execute vmxoff and exit from hypervisor
		auto guest_regs = reinterpret_cast<guest_registers*>(args);
		guest_regs->rax = vmx::vmx_handle_vmcall(guest_regs->rcx, guest_regs->rdx, guest_regs->r8, guest_regs->r9);
	}

	auto handle_execute_vmptrld(void* args) -> void {
		UNREFERENCED_PARAMETER(args);
		NT_ASSERTMSG("ARGS == NULL", args != nullptr);
		// TODO:
		vmx_vmexit_instruction_info_vmx_and_xsaves exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}

	auto handle_execute_vmptrst(void* args) -> void {
		UNREFERENCED_PARAMETER(args);
		NT_ASSERTMSG("ARGS == NULL", args != nullptr);
		// TODO:
		vmx_vmexit_instruction_info_vmx_and_xsaves exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}

	auto handle_execute_vmread(void* args) -> void {
		UNREFERENCED_PARAMETER(args);
		NT_ASSERTMSG("ARGS == NULL", args != nullptr);
		// TODO:
		vmx_vmexit_instruction_info_vmread_vmwrite exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}

	auto handle_execute_vmwrite(void* args) -> void {
		UNREFERENCED_PARAMETER(args);
		NT_ASSERTMSG("ARGS == NULL", args != nullptr);
		// TODO:
		vmx_vmexit_instruction_info_vmread_vmwrite exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}

	auto handle_execute_vmxon(void* args) -> void {
		UNREFERENCED_PARAMETER(args);
		NT_ASSERTMSG("ARGS == NULL", args != nullptr);
		// TODO:
		vmx_vmexit_instruction_info_vmx_and_xsaves exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}

	auto handle_mov_cr(void* args) -> void {
		NT_ASSERTMSG("ARGS == NULL", args != nullptr);
		auto guest_regs = reinterpret_cast<guest_registers*>(args);

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

	auto handle_mov_dr(void* args) -> void {
		UNREFERENCED_PARAMETER(args);
		NT_ASSERTMSG("ARGS == NULL", args != nullptr);
		// TODO:
		vmx_exit_qualification_mov_dr exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}

	auto handle_execute_io_instruction(void* args) -> void {
		UNREFERENCED_PARAMETER(args);
		NT_ASSERTMSG("ARGS == NULL", args != nullptr);
		// TODO:
		vmx_exit_qualification_io_instruction exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}

	auto handle_apic_access(void* args) -> void {
		UNREFERENCED_PARAMETER(args);
		NT_ASSERTMSG("ARGS == NULL", args != nullptr);
		// TODO:
		vmx_exit_qualification_apic_access exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}

	auto handle_gdtr_idtr_access(void* args) -> void {
		UNREFERENCED_PARAMETER(args);
		NT_ASSERTMSG("ARGS == NULL", args != nullptr);
		// TODO:
		vmx_vmexit_instruction_info_gdtr_idtr_access exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}

	auto handle_ldtr_tr_access(void* args) -> void {
		UNREFERENCED_PARAMETER(args);
		NT_ASSERTMSG("ARGS == NULL", args != nullptr);
		// TODO:
		vmx_vmexit_instruction_info_ldtr_tr_access exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}

	auto handle_ept_violation(void* args) -> void {
		UNREFERENCED_PARAMETER(args);
		//using ept::handle_ept_violation;

		__debugbreak();

		//auto guest_regs = reinterpret_cast<guest_registers*>(args);
		//vmx_exit_qualification_ept_violation exitQualification;
		//__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));

		//uint64_t phys_addr;
		//__vmx_vmread(VMCS_GUEST_PHYSICAL_ADDRESS, &phys_addr);
		//phys_addr = PAGE_ALIGN((PVOID)phys_addr);

		//uint64_t linear_addr;
		//__vmx_vmread(VMCS_EXIT_GUEST_LINEAR_ADDRESS, &linear_addr);

		//if (exitQualification.ept_executable || exitQualification.ept_readable || exitQualification.ept_writeable) {
			// These caused an EPT Violation
			//__debugbreak();
			//LOG("Error: VA = %llx, PA = %llx", linear_addr, phys_addr);
			//return;
		//}

		//if (!handle_ept_violation(phys_addr, linear_addr)) {
			//LOG("[!][%ws] Error handling apt violation\n", __FUNCTIONW__);
		//}
	}

	auto handle_ept_misconfiguration(void* args) -> void {
		UNREFERENCED_PARAMETER(args);
		NT_ASSERTMSG("ARGS == NULL", args != nullptr);
		// TODO:
		__debugbreak();

		// Failure setting EPT
		// Bugcheck and restart system
		KeBugCheck(PFN_LIST_CORRUPT);	// Is this bug code even correct??
	}

	auto handle_execute_invept(void* args) -> void {
		UNREFERENCED_PARAMETER(args);
		NT_ASSERTMSG("ARGS == NULL", args != nullptr);
		// TODO:
		vmx_vmexit_instruction_info_invalidate exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
		__debugbreak();
	}

	auto handle_execute_invvpid(void* args) -> void {
		UNREFERENCED_PARAMETER(args);
		NT_ASSERTMSG("ARGS == NULL", args != nullptr);
		// TODO:
		vmx_vmexit_instruction_info_invalidate exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}

	auto handle_execute_rdrand(void* args) -> void {
		UNREFERENCED_PARAMETER(args);
		NT_ASSERTMSG("ARGS == NULL", args != nullptr);
		// TODO:
		vmx_vmexit_instruction_info_rdrand_rdseed exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}

	auto handle_execute_invpcid(void* args) -> void {
		UNREFERENCED_PARAMETER(args);
		NT_ASSERTMSG("ARGS == NULL", args != nullptr);
		// TODO:
		vmx_vmexit_instruction_info_invalidate exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}

	auto handle_execute_xsaves(void* args) -> void {
		UNREFERENCED_PARAMETER(args);
		NT_ASSERTMSG("ARGS == NULL", args != nullptr);
		// TODO:
		vmx_vmexit_instruction_info_vmx_and_xsaves exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}

	auto handle_execute_xrstors(void* args) -> void {
		UNREFERENCED_PARAMETER(args);
		NT_ASSERTMSG("ARGS == NULL", args != nullptr);
		// TODO:
		vmx_vmexit_instruction_info_vmx_and_xsaves exitQualification;
		__vmx_vmread(VMCS_EXIT_QUALIFICATION, reinterpret_cast<size_t*>(&exitQualification));
	}
}
