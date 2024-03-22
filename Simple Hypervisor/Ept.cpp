#include "ept.hpp"

namespace ept {
	static mtrr_entry g_MtrrEntries[num_mtrr_entries];

	auto check_ept_support() -> bool {
		PAGED_CODE();

		PROCESSOR_NUMBER processor_number;
		GROUP_AFFINITY affinity, old_affinity;

		for (unsigned iter = 0; iter < g_num_processors; iter++) {
			KeGetProcessorNumberFromIndex(iter, &processor_number);

			RtlSecureZeroMemory(&affinity, sizeof(GROUP_AFFINITY));
			affinity.Group = processor_number.Group;
			affinity.Mask = static_cast<KAFFINITY>(1) << processor_number.Number;
			KeSetSystemGroupAffinityThread(&affinity, &old_affinity);

			auto irql = KeRaiseIrqlToDpcLevel();

			ia32_vmx_ept_vpid_cap_register ept_cap;
			ept_cap.flags = __readmsr(IA32_VMX_EPT_VPID_CAP);

			if (!ept_cap.page_walk_length_4 || !ept_cap.memory_type_write_back || !ept_cap.invept ||
				!ept_cap.invept_single_context || !ept_cap.invept_all_contexts || !ept_cap.invvpid ||
				!ept_cap.invvpid_individual_address || !ept_cap.invvpid_all_contexts ||
				!ept_cap.invvpid_single_context || !ept_cap.invvpid_single_context_retain_globals) {
				return false;
			}

			KeLowerIrql(irql);
			KeRevertToUserGroupAffinityThread(&old_affinity);
		}

		return true;
	}

	auto IsInRange(unsigned __int64 val, unsigned __int64 start, unsigned __int64 end) -> bool {
		if (val > start && val <= end)
			return true;

		return false;
	}

	auto ept_build_mtrr_map() -> bool {
		PAGED_CODE();

		mtrr_entry* _mtrr_entry = g_MtrrEntries;
		RtlSecureZeroMemory(_mtrr_entry, num_mtrr_entries * sizeof(mtrr_entry));

		ia32_mtrr_capabilities_register mtrr_cap;
		ia32_mtrr_def_type_register mtrr_def;
		ia32_mtrr_physbase_register mtrr_phys_base;
		ia32_mtrr_physmask_register mtrr_phys_mask;

		mtrr_cap.flags = __readmsr(IA32_MTRR_CAPABILITIES);
		mtrr_def.flags = __readmsr(IA32_MTRR_DEF_TYPE);

		uint64_t varCnt = mtrr_cap.variable_range_count;
		uint64_t FixRangeSupport = mtrr_cap.fixed_range_supported;
		uint64_t FixRangeEnable = mtrr_def.fixed_range_mtrr_enable;
		g_default_memory_type = mtrr_def.default_memory_type;

		if (FixRangeSupport && FixRangeEnable) {
			// Handle Fix Ranged MTRR

			static const uint64_t k64kBase = IA32_MTRR_FIX64K_BASE;
			static const uint64_t k64kManagedSize = IA32_MTRR_FIX64K_SIZE;	// 64K
			static const uint64_t k16kBase = IA32_MTRR_FIX16K_BASE;
			static const uint64_t k16kManagedSize = IA32_MTRR_FIX16K_SIZE;
			static const uint64_t k4kBase = IA32_MTRR_FIX4K_BASE;
			static const uint64_t k4kManagedSize = IA32_MTRR_FIX4K_SIZE;

			uint64_t offset = 0x0;

			// Let's first set Fix64K MTRR
			ia32_mtrr_fixed_range_msr msr64k;
			msr64k.all = __readmsr(IA32_MTRR_FIX64K_00000);
			for (unsigned idx = 0; idx < 8; idx++) {
				UINT64 base = k64kBase + offset;
				offset += k64kManagedSize;

				// Save the MTRR
				_mtrr_entry->mtrr_enabled = true;
				_mtrr_entry->memory_type = msr64k.fields.types[idx];
				_mtrr_entry->mtrr_fixed = true;
				_mtrr_entry->physical_address_start = base;
				_mtrr_entry->physical_address_end = base + k64kManagedSize - 1;

				g_mtrr_num += 1;
				_mtrr_entry++;
			}


			// let's set Fix16k MTRR
			ia32_mtrr_fixed_range_msr msr16k;
			// start -- IA32_MTRR_FIX16K_80000	
			// end   -- IA32_MTRR_FIX16K_A0000
			offset = 0;

			for (unsigned start = IA32_MTRR_FIX16K_80000; start <= IA32_MTRR_FIX16K_A0000; start++) {
				msr16k.all = __readmsr(start);
				for (unsigned idx = 0; idx < 8; idx++) {
					uint64_t base = k16kBase + offset;
					offset += k16kManagedSize;

					// Save the MTRR
					_mtrr_entry->memory_type = msr16k.fields.types[idx];
					_mtrr_entry->mtrr_enabled = true;
					_mtrr_entry->mtrr_fixed = true;
					_mtrr_entry->physical_address_start = base;
					_mtrr_entry->physical_address_end = base + k16kManagedSize - 1;

					g_mtrr_num += 1;
					_mtrr_entry++;
				}
			}


			// let's set Fix4k MTRR
			ia32_mtrr_fixed_range_msr msr4k;
			// start -- IA32_MTRR_FIX4K_C0000	
			// end   -- IA32_MTRR_FIX4K_F8000
			offset = 0;

			for (unsigned start = IA32_MTRR_FIX4K_C0000; start <= IA32_MTRR_FIX4K_F8000; start++) {
				msr4k.all = __readmsr(start);
				for (unsigned idx = 0; idx < 8; idx++) {
					uint64_t base = k4kBase + offset;
					offset += k4kManagedSize;

					// Save the MTRR
					_mtrr_entry->memory_type = msr4k.fields.types[idx];
					_mtrr_entry->mtrr_enabled = true;
					_mtrr_entry->mtrr_fixed = true;
					_mtrr_entry->physical_address_start = base;
					_mtrr_entry->physical_address_end = base + k4kManagedSize - 1;

					_mtrr_entry++;
				}
			}
		}

		for (unsigned iter = 0; iter < varCnt; iter++) {
			mtrr_phys_base.flags = __readmsr(IA32_MTRR_PHYSBASE0 + (iter * 2));
			mtrr_phys_mask.flags = __readmsr(IA32_MTRR_PHYSMASK0 + (iter * 2));

			// The range is invalid
			if (!mtrr_phys_mask.valid)
				continue;

			// Get the length this MTRR manages
			ULONG length;
			BitScanForward64(&length, mtrr_phys_mask.page_frame_number * PAGE_SIZE);

			uint64_t physical_base_start, physical_base_end;
			physical_base_start = mtrr_phys_base.page_frame_number * PAGE_SIZE;
			physical_base_end = physical_base_start + ((1ull << length) - 1);

			_mtrr_entry->mtrr_enabled = true;
			_mtrr_entry->mtrr_fixed = false;
			_mtrr_entry->memory_type = mtrr_phys_base.type;
			_mtrr_entry->physical_address_start = physical_base_start;
			_mtrr_entry->physical_address_end = physical_base_end;
			g_mtrr_num++;
			_mtrr_entry++;
		}

		return true;
	}

	auto InitializeEpt(unsigned char processor_number) -> bool {
		PAGED_CODE();

		ept_state* _ept_state = reinterpret_cast<ept_state*>
			(ExAllocatePoolWithTag(NonPagedPool, sizeof(ept_state), VMM_POOL_TAG));
		if (!_ept_state) {
			LOG("[-] Failed to allocate memory for Ept State.\n");
			LOG_ERROR();
			return false;
		}
		RtlSecureZeroMemory(_ept_state, sizeof ept_state);

		ept_pointer* ept_ptr = reinterpret_cast<ept_pointer*>
			(ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, VMM_POOL_TAG));
		if (!ept_ptr) {
			LOG("[-] Failed to allocate memory for pointer to Ept.\n");
			LOG_ERROR();
			return false;
		}
		_ept_state->ept_ptr = ept_ptr;
		RtlSecureZeroMemory(ept_ptr, PAGE_SIZE);


		if (create_ept_state(_ept_state) == false) {
			DbgPrint("[-] Failed to set Ept Page Table Entries.\n");
			LOG_ERROR();
			ExFreePoolWithTag(ept_ptr, VMM_POOL_TAG);
			ExFreePoolWithTag(_ept_state, VMM_POOL_TAG);
			return false;
		}

		vmm_context[processor_number].ept_ptr = _ept_state->ept_ptr->flags;
		vmm_context[processor_number].ept_state = _ept_state;


		LOG("[*] EPT initialized on processor (%x)\n", processor_number);
		return true;
	}
}
