/*
 * Copyright (c) Martin Kinkelin
 *
 * See the "License.txt" file in the root directory for infos
 * about permitted and prohibited uses of this code.
 */

#include <algorithm>
#include <iostream>
#include <locale>
#include "Worker.h"
#include "StringUtils.h"
#include "WinRing0.h"

using std::cerr;
using std::endl;
using std::min;
using std::max;
using std::string;
using std::tolower;
using std::vector;


static void SplitPair(string& left, string& right, const string& str, char delimiter)
{
	const size_t i = str.find(delimiter);

	left = str.substr(0, i);

	if (i == string::npos)
		right.clear();
	else
		right = str.substr(i + 1);

}

bool Worker::ParseParams(int argc, const char* argv[])
{
	const Info& info = *_info;

	// scale factor from external multi to internal one (100 MHz reference)
	const double multiScaleFactor = (info.Family == 0x12 || info.Family == 0x14 ? 1.0 : 2.0);

	PStateInfo psi;
	psi.Multi = psi.VID = psi.NBVID = -1.0;
	psi.NBPState = -1;

	NBPStateInfo nbpsi;
	nbpsi.Multi = nbpsi.VID = -1.0;

	for (int i = 0; i < info.NumPStates; i++)
	{
		_pStates.push_back(psi);
		_pStates.back().Index = i;
	}
	for (int i = 0; i < 2; i++)
	{
		_nbPStates.push_back(nbpsi);
		_nbPStates.back().Index = i;
	}

	for (int i = 1; i < argc; i++)
	{
		const string param(argv[i]);

		string key, value;
		SplitPair(key, value, param, '=');

		if (value.empty())
		{
			if (param.length() >= 2 && tolower(param[0]) == 'p')
			{
				const int index = atoi(param.c_str() + 1);
				if (index >= 0 && index < info.NumPStates)
				{
					_pState = index;
					continue;
				}
			}
		}
		else
		{
			if (key.length() >= 2 && tolower(key[0]) == 'p')
			{
				const int index = atoi(key.c_str() + 1);
				if (index >= 0 && index < info.NumPStates)
				{
					string multi, vid;
					SplitPair(multi, vid, value, '@');

					if (!multi.empty())
						_pStates[index].Multi = multiScaleFactor * atof(multi.c_str());
					if (!vid.empty())
						_pStates[index].VID = atof(vid.c_str());

					continue;
				}
			}

			if (key.length() >= 5 && _strnicmp(key.c_str(), "NB_P", 4) == 0)
			{
				const int index = atoi(key.c_str() + 4);
				if (index >= 0 && index < 2)
				{
					string multi, vid;
					SplitPair(multi, vid, value, '@');

					if (!multi.empty())
						_nbPStates[index].Multi = atof(multi.c_str());
					if (!vid.empty())
						_nbPStates[index].VID = atof(vid.c_str());

					continue;
				}
			}

			if (_stricmp(key.c_str(), "NB_low") == 0)
			{
				const int index = atoi(value.c_str());

				int j = 0;
				for (; j < min(index, info.NumPStates); j++)
					_pStates[j].NBPState = 0;
				for (; j < info.NumPStates; j++)
					_pStates[j].NBPState = 1;

				continue;
			}

			if (_stricmp(key.c_str(), "Turbo") == 0)
			{
				const int flag = atoi(value.c_str());
				if (flag == 0 || flag == 1)
				{
					_turbo = flag;
					continue;
				}
			}

			if (_stricmp(key.c_str(), "APM") == 0)
			{
				const int flag = atoi(value.c_str());
				if (flag == 0 || flag == 1)
				{
					_apm = flag;
					continue;
				}
			}
		}

		cerr << "ERROR: invalid parameter " << param.c_str() << endl;
		return false;
	}

	return true;
}


static bool ContainsChanges(const PStateInfo& info)
{
	return (info.Multi >= 0 || info.VID >= 0 || info.NBVID >= 0 || info.NBPState >= 0);
}
static bool ContainsChanges(const NBPStateInfo& info)
{
	return (info.Multi >= 0 || info.VID >= 0);
}

static void SwitchTo(int logicalCPUIndex)
{
	const HANDLE hThread = GetCurrentThread();
	SetThreadAffinityMask(hThread, (DWORD_PTR)1 << logicalCPUIndex);
}

void Worker::ApplyChanges()
{
	const Info& info = *_info;

	if (info.Family == 0x15)
	{
		for (int i = 0; i < _nbPStates.size(); i++)
		{
			const NBPStateInfo& nbpsi = _nbPStates[i];
			if (ContainsChanges(nbpsi))
				info.WriteNBPState(nbpsi);
		}
	}
	else if (info.Family == 0x10 && (_nbPStates[0].VID >= 0 || _nbPStates[1].VID >= 0))
	{
		for (int i = 0; i < _pStates.size(); i++)
		{
			PStateInfo& psi = _pStates[i];

			const int nbPState = (psi.NBPState >= 0 ? psi.NBPState :
			                                          info.ReadPState(i).NBPState);
			const NBPStateInfo& nbpsi = _nbPStates[nbPState];

			if (nbpsi.VID >= 0)
				psi.NBVID = nbpsi.VID;
		}
	}

	if (_turbo >= 0 && info.IsBoostSupported)
		info.SetBoostSource(_turbo == 1);
	if (_apm >= 0 && info.Family == 0x15)
		info.SetAPM(_apm == 1);

	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	const int numLogicalCPUs = sysInfo.dwNumberOfProcessors;

	// switch to the highest thread priority (we do not want to get interrupted often)
	const HANDLE hProcess = GetCurrentProcess();
	const HANDLE hThread = GetCurrentThread();
	SetPriorityClass(hProcess, REALTIME_PRIORITY_CLASS);
	SetThreadPriority(hThread, THREAD_PRIORITY_HIGHEST);

	// perform one iteration in each logical core
	for (int j = 0; j < numLogicalCPUs; j++)
	{
		SwitchTo(j);

		for (int i = 0; i < _pStates.size(); i++)
		{
			const PStateInfo& psi = _pStates[i];
			if (ContainsChanges(psi))
				info.WritePState(psi);
		}

		if (_turbo >= 0 && info.IsBoostSupported)
			info.SetCPBDis(_turbo == 1);
	}

	for (int j = 0; j < numLogicalCPUs; j++)
	{
		SwitchTo(j);

		const int currentPState = info.GetCurrentPState();
		const int newPState = (_pState >= 0 ? _pState : currentPState);

		if (newPState != currentPState)
			info.SetCurrentPState(newPState);
		else
		{
			if (ContainsChanges(_pStates[currentPState]))
			{
				const int tempPState = (currentPState == info.NumPStates - 1 ? 0 : info.NumPStates - 1);
				info.SetCurrentPState(tempPState);
				Sleep(1);
				info.SetCurrentPState(currentPState);
			}
		}
	}

	SetThreadPriority(hThread, THREAD_PRIORITY_NORMAL);
	SetPriorityClass(hProcess, NORMAL_PRIORITY_CLASS);
}
