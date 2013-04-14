/*
 * Copyright (c) Martin Kinkelin
 *
 * See the "License.txt" file in the root directory for infos
 * about permitted and prohibited uses of this code.
 */

#pragma comment(lib, "WinRing0/WinRing0.lib")
#pragma comment(lib, "WinRing0/WinRing0x64.lib")

#include "WinRing0.h"
#include "StringUtils.h"

using std::exception;
using std::string;


DWORD ReadPciConfig(DWORD device, DWORD function, DWORD regAddress)
{
	const DWORD pciAddress = ((device & 0x1f) << 3) | (function & 0x7);

	DWORD result;
	if (!ReadPciConfigDwordEx(pciAddress, regAddress, &result))
	{
		string msg = "cannot read from PCI configuration space (F";
		msg += StringUtils::ToString(function);
		msg += "x";
		msg += StringUtils::ToHexString(regAddress);
		msg += ")";

		throw exception(msg.c_str());
	}

	return result;
}

void WritePciConfig(DWORD device, DWORD function, DWORD regAddress, DWORD value)
{
	const DWORD pciAddress = ((device & 0x1f) << 3) | (function & 0x7);

	if (!WritePciConfigDwordEx(pciAddress, regAddress, value))
	{
		string msg = "cannot write to PCI configuration space (F";
		msg += StringUtils::ToString(function);
		msg += "x";
		msg += StringUtils::ToHexString(regAddress);
		msg += ")";

		throw exception(msg.c_str());
	}
}


QWORD Rdmsr(DWORD index)
{
	QWORD result;
	PDWORD eax = (PDWORD)&result;
	PDWORD edx = eax + 1;

	if (!Rdmsr(index, eax, edx))
	{
		string msg = "cannot read from MSR (0x";
		msg += StringUtils::ToHexString(index);
		msg += ")";

		throw exception(msg.c_str());
	}

	return result;
}

void Wrmsr(DWORD index, const QWORD& value)
{
	PDWORD eax = (PDWORD)&value;
	PDWORD edx = eax + 1;

	if (!Wrmsr(index, *eax, *edx))
	{
		string msg = "cannot write to MSR (0x";
		msg += StringUtils::ToHexString(index);
		msg += ")";

		throw exception(msg.c_str());
	}
}


CpuidRegs Cpuid(DWORD index)
{
	CpuidRegs result;
	if (!Cpuid(index, &result.eax, &result.ebx, &result.ecx, &result.edx))
	{
		string msg = "cannot execute CPUID instruction (0x";
		msg += StringUtils::ToHexString(index);
		msg += ")";

		throw exception(msg.c_str());
	}

	return result;
}
