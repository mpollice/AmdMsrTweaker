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
	int VID;
	int NBPState;
	int NBVID; // family 0x10 only
};

struct NBPStateInfo
{
	int Index;
	double Multi; // for 200 MHz reference
	int VID;
};


class Info
{
public:

	int Family;
	int Model;
	int NumCores;
	int NumPStates;
	int NumNBPStates;

	double MinMulti, MaxMulti; // internal ones for 100 MHz reference
	double MaxSoftwareMulti; // for software (i.e., non-boost) P-states
	double MinVID, MaxVID;
	double VIDStep;
	double multiScaleFactor;

	bool IsBoostSupported;
	bool IsBoostEnabled;
	bool IsBoostLocked;
	int NumBoostStates;


	Info()
		: Family(0)
		, Model(0)
		, NumCores(0)
		, NumPStates(0)
		, NumNBPStates(2) //except family 0x15, we have at least 2 NB P-States
		, MinMulti(0.0), MaxMulti(0.0)
		, MaxSoftwareMulti(0.0)
		, MinVID(0.0), MaxVID(0.0)
		, VIDStep(0.0125) //default step for pre SVI2 platforms
		, multiScaleFactor(1.0) //default for 100MHz REFCLK
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

	double DecodeVID(int vid) const;
	int EncodeVID(double vid) const;

private:

	double DecodeMulti(int fid, int did) const;
	void EncodeMulti(double multi, int& fid, int& did) const;

};
