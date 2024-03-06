//#include "vmcs.hpp"
#include "stdafx.h"

auto setup_vmcs(unsigned long processor_number, void* guest_rsp) -> EVmErrors {
	__debugbreak();
	UNREFERENCED_PARAMETER(processor_number);
	UNREFERENCED_PARAMETER(guest_rsp);

	return VM_ERROR_OK;

}
