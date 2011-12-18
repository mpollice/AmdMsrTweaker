/*
 * Copyright (c) Martin Kinkelin
 *
 * See the "License.txt" file in the root directory for infos
 * about permitted and prohibited uses of this code.
 */

#pragma once

#define WIN32_MEAN_AND_LEAN
#include <windows.h>
#undef min
#undef max

#include "WinRing0/OlsApi.h"


typedef unsigned long long QWORD;

struct CpuidRegs
{
	DWORD eax;
	DWORD ebx;
	DWORD ecx;
	DWORD edx;
};


DWORD ReadPciConfig(DWORD function, DWORD regAddress);
void WritePciConfig(DWORD function, DWORD regAddress, DWORD value);

QWORD Rdmsr(DWORD index);
void Wrmsr(DWORD index, const QWORD& value);

CpuidRegs Cpuid(DWORD index);


template <typename T> DWORD GetBits(T value, unsigned char offset, unsigned char numBits)
{
	const T mask = (((T)1 << numBits) - (T)1); // 2^numBits - 1; after right-shift
	return (DWORD)((value >> offset) & mask);
}

template <typename T> void SetBits(T& value, DWORD bits, unsigned char offset, unsigned char numBits)
{
	const T mask = (((T)1 << numBits) - (T)1) << offset; // 2^numBits - 1, shifted by offset to the left
	value = (value & ~mask) | (((T)bits << offset) & mask);
}
