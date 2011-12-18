/*
 * Copyright (c) Martin Kinkelin
 *
 * See the "License.txt" file in the root directory for infos
 * about permitted and prohibited uses of this code.
 */

#include <iostream>
#include <conio.h>
#include "Info.h"
#include "Worker.h"
#include "WinRing0.h"

using std::cout;
using std::cerr;
using std::endl;


void PrintInfo(const Info& info);
void WaitForKey();


/// <summary>Entry point for the program.</summary>
int main(int argc, const char* argv[])
{
	// initialize WinRing0
	if (!InitializeOls() || GetDllStatus() != 0)
	{
		cerr << "ERROR: WinRing0 initialization failed" << endl;
		DeinitializeOls();

		return 1;
	}

	try
	{
		Info info;
		if (!info.Initialize())
		{
			cout << "ERROR: unsupported CPU" << endl;
			DeinitializeOls();
			WaitForKey();
			return 2;
		}

		if (argc > 1)
		{
			Worker worker(info);

			if (!worker.ParseParams(argc, argv))
			{
				DeinitializeOls();
				WaitForKey();
				return 3;
			}

			worker.ApplyChanges();
		}
		else
		{
			PrintInfo(info);
			WaitForKey();
		}
	}
	catch (const std::exception& e)
	{
		cerr << "ERROR: " << e.what() << endl;
		DeinitializeOls();
		WaitForKey();
		return 10;
	}

	DeinitializeOls();

	return 0;
}


void PrintInfo(const Info& info)
{
	// scale factor from internal multi (100 MHz reference) to external one
	const double multiScaleFactor = (info.Family == 0x12 || info.Family == 0x14 ? 1.0 : 0.5);

	cout << endl;
	cout << "AmdMsrTweaker" << endl;
	cout << endl;

	cout << ".:. General" << endl << "---" << endl;
	cout << "  AMD family 0x" << std::hex << info.Family << std::dec << " CPU, " << info.NumCores << " cores" << endl;
	cout << "  Available multipliers: " << (multiScaleFactor * info.MinMulti) << " .. " << (multiScaleFactor * info.MaxSoftwareMulti) << endl;
	cout << "  Available voltage IDs: " << info.MinVID << " .. " << info.MaxVID << endl;
	cout << endl;

	cout << ".:. Turbo" << endl << "---" << endl;
	if (!info.IsBoostSupported)
		cout << "  not supported" << endl;
	else
	{
		cout << "  " << (info.IsBoostEnabled ? "enabled" : "disabled") << endl;
		cout << "  " << (info.IsBoostLocked ? "locked" : "unlocked") << endl;

		if (info.MaxMulti != info.MaxSoftwareMulti)
			cout << "  Max multiplier: " << (multiScaleFactor * info.MaxMulti) << endl;
	}
	cout << endl;

	cout << ".:. P-states" << endl << "---" << endl;
	cout << "  " << info.NumPStates << " of " << (info.Family == 0x10 ? 5 : 8) << " enabled (P0 .. P" << (info.NumPStates - 1) << ")" << endl;

	if (info.IsBoostSupported && info.NumBoostStates > 0)
	{
		cout << "  Turbo P-states:";
		for (int i = 0; i < info.NumBoostStates; i++)
			cout << " P" << i;
		cout << endl;
	}

	cout << "  ---" << endl;

	for (int i = 0; i < info.NumPStates; i++)
	{
		const PStateInfo pi = info.ReadPState(i);

		cout << "  P" << i << ": " << (multiScaleFactor * pi.Multi) << "x at " << pi.VID << "V" << endl;

		if (pi.NBPState >= 0)
		{
			cout << "      NorthBridge in NB_P" << pi.NBPState;
			if (pi.NBVID >= 0)
				cout << " at " << pi.NBVID << "V";
			cout << endl;
		}
	}

	if (info.Family == 0x15)
	{
		cout << "  ---" << endl;

		DWORD eax = ReadPciConfig(0xf5, 0x170);
		int numNBPStates = (eax & 0x3) + 1;

		for (int i = 0; i < numNBPStates; i++)
		{
			const NBPStateInfo pi = info.ReadNBPState(i);
			cout << "  NB_P" << i << ": " << pi.Multi << "x at " << pi.VID << "V" << endl;
		}
	}
}


void WaitForKey()
{
	cout << endl << "Press any key to exit... ";
	_getch();
	cout << endl;
}
