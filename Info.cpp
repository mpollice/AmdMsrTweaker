/*
 * Copyright (c) Martin Kinkelin
 *
 * See the "License.txt" file in the root directory for infos
 * about permitted and prohibited uses of this code.
 */

#include <algorithm> // for min/max
#include <exception>
#include "Info.h"
#include "WinRing0.h"

using std::min;
using std::max;

// divisors for families 0x10 and 0x15
static const double DIVISORS_10_15[] = { 1.0, 2.0, 4.0, 8.0, 16.0, 0.0 };
// special divisors for family 0x12
static const double DIVISORS_12[] = { 1.0, 1.5, 2.0, 3.0, 4.0, 6.0, 8.0, 12.0, 16.0, 0.0 };


void FindFraction(double value, const double* divisors,
	int& numerator, int& divisorIndex,
	int minNumerator, int maxNumerator);


bool Info::Initialize()
{
	CpuidRegs regs;
	QWORD msr;
	DWORD eax;

	// verify vendor = AMD ("AuthenticAMD")
	regs = Cpuid(0x80000000);
	if (regs.ecx != 0x444d4163) // "DMAc"
		return false;

	// check family
	regs = Cpuid(0x80000001);
	Family = GetBits(regs.eax, 8, 4) + GetBits(regs.eax, 20, 8);
	if (!(Family == 0x10 || Family == 0x12 || Family == 0x14 || Family == 0x15))
		return false;

	// read model
	Model = GetBits(regs.eax, 4, 4) + (GetBits(regs.eax, 16, 4) << 4);

	//set VID step for SVI2 platforms (otherwise 0.0125 is assumed, see header)
	// Family 0x15 Models 10-1F is Trinity/Richland
	// Family 0x15 Models 30-3F is Kaveri
	if (Family == 0x15 && ((Model > 0xF && Model < 0x20) || (Model > 0x2F && Model < 0x40)))
		VIDStep = 0.00625;

	// scale factor from external multi to internal one (default 1, set for 200MHz REFCLK platforms)
	// Family 0x10 includes all AM2+/AM3 K10 CPUs
	// Family 0x15 Models 0-F is Bulldozer/Piledriver
	if (Family == 0x10 || (Family == 0x15 && Model < 0x10))
		multiScaleFactor = 2.0;

	// number of physical cores
	regs = Cpuid(0x80000008);
	NumCores = GetBits(regs.ecx, 0, 8) + 1;

	// number of hardware P-states
	eax = ReadPciConfig(3, 0xdc);
	NumPStates = GetBits(eax, 8, 3) + 1;

	if (Family == 0x15)
	{
		eax = ReadPciConfig(0xf5, 0x170);
		NumNBPStates = (eax & 0x3) + 1;
	}

	// get limits
	msr = Rdmsr(0xc0010071);

	const int maxMulti = GetBits(msr, 49, 6);
	const int minVID = GetBits(msr, 42, 7);
	const int maxVID = GetBits(msr, 35, 7);

	MinMulti = (Family == 0x14 ? (maxMulti == 0 ? 0 : (maxMulti + 16) / 26.5)
	                           : 1.0);
	MaxMulti = (maxMulti == 0 ? (Family == 0x14 ? 0
	                                            : (Family == 0x12 ? 31 + 16 : 47 + 16))
	                          : (Family == 0x12 || Family == 0x14 ? maxMulti + 16 : maxMulti));
	MaxSoftwareMulti = MaxMulti;

	MinVID = (minVID == 0 ? 0.0
	                      : DecodeVID(minVID));
	MaxVID = (maxVID == 0 ? 1.55
	                      : DecodeVID(maxVID));

	// is CBP (core performance boost) supported?
	regs = Cpuid(0x80000007);
	IsBoostSupported = (GetBits(regs.edx, 9, 1) == 1);

	if (IsBoostSupported)
	{
		// is CPB disabled for the current core?
		msr = Rdmsr(0xc0010015);
		const bool cpbDis = (GetBits(msr, 25, 1) == 1);

		// boost lock, number of boost P-states and boost source
		eax = ReadPciConfig(4, 0x15c);
		IsBoostLocked = (Family == 0x12 ? true
		                                : GetBits(eax, 31, 1) == 1);
		NumBoostStates = (Family == 0x10 ? GetBits(eax, 2, 1)
		                                 : GetBits(eax, 2, 3));
		const int boostSrc = GetBits(eax, 0, 2);
		const bool isBoostSrcEnabled = (Family == 0x10 ? (boostSrc == 3)
		                                               : (boostSrc == 1));

		IsBoostEnabled = (isBoostSrcEnabled && !cpbDis);

		// max multi for software P-states (families 0x10 and 0x15)
		if (Family == 0x10)
		{
			eax = ReadPciConfig(3, 0x1f0);
			const int maxSoftwareMulti = GetBits(eax, 20, 6);
			MaxSoftwareMulti = (maxSoftwareMulti == 0 ? 63
			                                          : maxSoftwareMulti);
		}
		else if (Family == 0x15)
		{
			eax = ReadPciConfig(3, 0xd4);
			const int maxSoftwareMulti = GetBits(eax, 0, 6);
			MaxSoftwareMulti = (maxSoftwareMulti == 0 ? 63
			                                          : maxSoftwareMulti);
		}
	}

	return true;
}



PStateInfo Info::ReadPState(int index) const
{
	const QWORD msr = Rdmsr(0xc0010064 + index);

	PStateInfo result;
	result.Index = index;

	int fid, did;
	if (Family == 0x14)
	{
		fid = GetBits(msr, 4, 5); // DID MSD
		did = GetBits(msr, 0, 4); // DID LSD
	}
	else if (Family == 0x12)
	{
		fid = GetBits(msr, 4, 5);
		did = GetBits(msr, 0, 4);
	}
	else
	{
		fid = GetBits(msr, 0, 6);
		did = GetBits(msr, 6, 3);
	}

	result.Multi = DecodeMulti(fid, did);

	//on SVI2 platforms, VID is 8 bits
	if (Family == 0x15 && ((Model > 0xF && Model < 0x20) || (Model > 0x2F && Model < 0x40)))
		result.VID = GetBits(msr, 9, 8);
	else
		result.VID = GetBits(msr, 9, 7);

	if (!(Family == 0x12 || Family == 0x14))
	{
		const int nbDid = GetBits(msr, 22, 1);
		result.NBPState = nbDid;
	}
	else
		result.NBPState = -1;

	if (Family == 0x10)
	{
		result.NBVID = GetBits(msr, 25, 7);
	}
	else
		result.NBVID = -1;

	return result;
}

void Info::WritePState(const PStateInfo& info) const
{
	const DWORD regIndex = 0xc0010064 + info.Index;
	QWORD msr = Rdmsr(regIndex);

	if (info.Multi >= 0)
	{
		int fid, did;
		EncodeMulti(info.Multi, fid, did);

		if (Family == 0x14)
		{
			SetBits(msr, fid, 4, 5); // DID MSD
			SetBits(msr, did, 0, 4); // DID LSD
		}
		else if (Family == 0x12)
		{
			SetBits(msr, fid, 4, 5);
			SetBits(msr, did, 0, 4);
		}
		else
		{
			SetBits(msr, fid, 0, 6);
			SetBits(msr, did, 6, 3);
		}
	}

	if (info.VID >= 0)
	{
		//on SVI2 platforms, VID is 8 bits
		if (Family == 0x15 && ((Model > 0xF && Model < 0x20) || (Model > 0x2F && Model < 0x40)))
			SetBits(msr, info.VID, 9, 8);
		else
			SetBits(msr, info.VID, 9, 7);
	}

	if (info.NBPState >= 0)
	{
		if (!(Family == 0x12 || Family == 0x14))
		{
			const int nbDid = max(0, min(1, info.NBPState));
			SetBits(msr, nbDid, 22, 1);
		}
	}

	if (info.NBVID >= 0)
	{
		if (Family == 0x10)
		{
			SetBits(msr, info.NBVID, 25, 7);
		}
	}

	Wrmsr(regIndex, msr);
}



NBPStateInfo Info::ReadNBPState(int index) const
{
	if (Family != 0x15)
		throw std::exception("NB P-states not supported");

	NBPStateInfo result;
	result.Index = index;

	const DWORD eax = ReadPciConfig(5, 0x160 + index * 4);

	const int fid = GetBits(eax, 1, 5);
	const int did = GetBits(eax, 7, 1);
	int vid = GetBits(eax, 10, 7);

	//on SVI2 platforms, 8th bit for NB P-State is stored separately
	if (Family == 0x15 && ((Model > 0xF && Model < 0x20) || (Model > 0x2F && Model < 0x40)))
		vid += (GetBits(eax, 21, 1) << 7);

	result.Multi = (fid + 4) / pow(2.0, did);
	result.VID = vid;

	return result;
}

void Info::WriteNBPState(const NBPStateInfo& info) const
{
	if (Family != 0x15)
		throw std::exception("NB P-states not supported");

	const DWORD regAddress = 0x160 + info.Index * 4;
	DWORD eax = ReadPciConfig(5, regAddress);

	if (info.Multi >= 0)
	{
		static const double divisors[] = { 1.0, 2.0, 0.0 }; // 2^did

		int numerator, divisorIndex;
		FindFraction(info.Multi, divisors, numerator, divisorIndex, 4, 31 + 4);

		const int fid = numerator - 4;
		const int did = divisorIndex;

		SetBits(eax, fid, 1, 5);
		SetBits(eax, did, 7, 1);
	}

	if (info.VID >= 0)
	{
		SetBits(eax, info.VID, 10, 7);

		//on SVI2 platforms, 8th bit for NB P-State is stored separately
		if (Family == 0x15 && ((Model > 0xF && Model < 0x20) || (Model > 0x2F && Model < 0x40)))
			SetBits(eax, (info.VID >> 7), 21, 1);
	}

	WritePciConfig(5, regAddress, eax);
}


void Info::SetCPBDis(bool enabled) const
{
	if (!IsBoostSupported)
		throw std::exception("CPB not supported");

	const DWORD index = 0xc0010015;
	QWORD msr = Rdmsr(index);
	SetBits(msr, (enabled ? 0 : 1), 25, 1);
	Wrmsr(index, msr);
}

void Info::SetBoostSource(bool enabled) const
{
	if (!IsBoostSupported)
		throw std::exception("CPB not supported");

	DWORD eax = ReadPciConfig(4, 0x15c);
	const int bits = (enabled ? (Family == 0x10 ? 3 : 1)
	                          : 0);
	SetBits(eax, bits, 0, 2);
	WritePciConfig(4, 0x15c, eax);
}

void Info::SetAPM(bool enabled) const
{
	if (Family != 0x15)
		throw std::exception("APM not supported");

	DWORD eax = ReadPciConfig(4, 0x15c);
	SetBits(eax, (enabled ? 1 : 0), 7, 1);
	WritePciConfig(4, 0x15c, eax);
}


int Info::GetCurrentPState() const
{
	const QWORD msr = Rdmsr(0xc0010071);
	const int i = GetBits(msr, 16, 3);
	return i;
}

void Info::SetCurrentPState(int index) const
{
	if (index < 0 || index >= NumPStates)
		throw std::exception("P-state index out of range");

	index -= NumBoostStates;
	if (index < 0)
		index = 0;

	const DWORD regIndex = 0xc0010062;
	QWORD msr = Rdmsr(regIndex);
	SetBits(msr, index, 0, 3);
	Wrmsr(regIndex, msr);
}



double Info::DecodeMulti(int fid, int did) const
{
	if (Family == 0x14)
	{
		// fid => DID MSD (integral part of divisor - 1)
		// did => DID LSD (fractional part of divisor, in quarters)

		double divisor = fid + 1;

		if (divisor >= 16)
			did &= ~1; // ignore least significant bit of LSD
		divisor += did * 0.25;

		return MaxMulti / divisor;
	}

	const double* divisors = (Family == 0x12 ? DIVISORS_12
	                                         : DIVISORS_10_15);

	return (fid + 16) / divisors[did];
}

void Info::EncodeMulti(double multi, int& fid, int& did) const
{
	if (Family == 0x14)
	{
		if (MaxMulti == 0)
			throw std::exception("cannot encode multiplier (family 0x14) - unknown max multiplier");

		const double exactDivisor = max(1.0, min(26.5, MaxMulti / multi));

		double integer;
		const double fractional = modf(exactDivisor, &integer);

		fid = (int)integer - 1;

		did = (int)ceil(fractional / 0.25);

		if (integer >= 16)
		{
			if (did == 1)
				did = 2;
			else if (did == 3)
				did = 4;
		}

		if (did == 4)
		{
			fid++;
			did = 0;
		}

		return;
	}

	const int minNumerator = 16; // numerator: 0x10 = 16 as fixed offset
	int maxNumerator;
	const double* divisors;

	if (Family == 0x12)
	{
		maxNumerator = 31 + minNumerator; // 5 bits => max 2^5-1 = 31
		divisors = DIVISORS_12;
	}
	else
	{
		maxNumerator = 47 + minNumerator; // 6 bits, but max 0x2f = 47
		divisors = DIVISORS_10_15;
	}

	int numerator, divisorIndex;
	FindFraction(multi, divisors, numerator, divisorIndex, minNumerator, maxNumerator);

	fid = numerator - minNumerator;
	did = divisorIndex;
}


double Info::DecodeVID(int vid) const
{
	return 1.55 - vid * VIDStep;
}

int Info::EncodeVID(double vid) const
{
	vid = max(0.0, min(1.55, vid));

	// round to nearest step
	int r = (int)(vid / VIDStep + 0.5);

	//1.55 / VIDStep = highest VID (0 V)
	return (int)(1.55 / VIDStep) - r;
}



void FindFraction(double value, const double* divisors,
	int& numerator, int& divisorIndex,
	int minNumerator, int maxNumerator)
{
	// limitations: non-negative value and divisors

	// count the null-terminated and ascendingly ordered divisors
	int numDivisors = 0;
	for (; divisors[numDivisors] > 0; numDivisors++) { }

	// make sure the value is in a valid range
	value = max(minNumerator / divisors[numDivisors-1], min(maxNumerator / divisors[0], value));

	// search the best-matching combo
	double bestValue = -1.0; // numerator / divisors[divisorIndex]
	for (int i = 0; i < numDivisors; i++)
	{
		const double d = divisors[i];
		const int n = max(minNumerator, min(maxNumerator, (int)(value * d)));
		const double myValue = n / d;

		if (myValue <= value && myValue > bestValue)
		{
			numerator = n;
			divisorIndex = i;
			bestValue = myValue;

			if (bestValue == value)
				break;
		}
	}
}
