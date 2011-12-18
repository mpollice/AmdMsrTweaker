/*
 * Copyright (c) Martin Kinkelin
 *
 * See the "License.txt" file in the root directory for infos
 * about permitted and prohibited uses of this code.
 */

#pragma once


struct PStateInfo
{
	int Index;    // hardware index
	double Multi; // internal one for 100 MHz reference
	double VID;
	int NBPState;
	double NBVID; // family 0x10 only
};

struct NBPStateInfo
{
	int Index;
	double Multi; // for 200 MHz reference
	double VID;
};


class Info
{
public:

	int Family;
	int NumCores;
	int NumPStates;

	double MinMulti, MaxMulti; // internal ones for 100 MHz reference
	double MaxSoftwareMulti; // for software (i.e., non-boost) P-states
	double MinVID, MaxVID;

	bool IsBoostSupported;
	bool IsBoostEnabled;
	bool IsBoostLocked;
	int NumBoostStates;


	Info()
		: Family(0)
		, NumCores(0)
		, NumPStates(0)
		, MinMulti(0.0), MaxMulti(0.0)
		, MaxSoftwareMulti(0.0)
		, MinVID(0.0), MaxVID(0.0)
		, IsBoostSupported(false)
		, IsBoostEnabled(false)
		, IsBoostLocked(false)
		, NumBoostStates(0)
	{
	}

	bool Initialize();

	PStateInfo ReadPState(int index) const;
	void WritePState(const PStateInfo& info) const;

	NBPStateInfo ReadNBPState(int index) const;
	void WriteNBPState(const NBPStateInfo& info) const;

	void SetCPBDis(bool enabled) const;
	void SetBoostSource(bool enabled) const;
	void SetAPM(bool enabled) const;

	int GetCurrentPState() const;
	void SetCurrentPState(int index) const;


private:

	double DecodeMulti(int fid, int did) const;
	void EncodeMulti(double multi, int& fid, int& did) const;

	double DecodeVID(int vid) const;
	int EncodeVID(double vid) const;
};
