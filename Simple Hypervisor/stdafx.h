#pragma once

#include <ntifs.h>
#include <ntddk.h>
#include <intrin.h>
#include <windef.h>

#include "Ept.hpp"
#include "Vmx.hpp"
#include "ia32.hpp"
#include "Vmcs.hpp"
#include "VmUtils.hpp"


using uint8_t = unsigned char;
using uint16_t = unsigned short;
using uint32_t = unsigned int;
using uint64_t = unsigned long long;

using int32_t = signed int;
using int64_t = signed long long;

using ushort = unsigned short;