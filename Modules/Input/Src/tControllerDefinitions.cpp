// tControllerDefinitions.cpp
//
// This file contains a table specifying the properties of various controller models. In particular the controller
// properties may be looked up if you supply the vendor ID and the product ID. The suspected polling period, a
// descriptive name, component technology used, plus latency and jitter information are all included. The data is based
// on https://gist.github.com/nondebug/aec93dff7f0f1969f4cc2291b24a3171 and https://gamepadla.com/
//
// Copyright (c) 2025 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <Foundation/tMap.h>
#include "Input/tControllerDefinitions.h"


namespace tInput
{
	// The controller dictionary. Lookup by vidpid and contains the controller definitions.
	tMap<tVidPid, tContDefn> ContDict;

	bool ContDictPopulated = false;
	void tPopulateContDict();
}


const tInput::tContDefn* tInput::tLookupContDefn(const tVidPid& vidpid)
{
	if (!ContDictPopulated)
	{
		tPopulateContDict();
		ContDictPopulated = true;
	}

	return ContDict.GetValue(vidpid);
}


void tInput::tPopulateContDict()
{
	ContDict[ {0x2DC8, 0x310B} ] =
	{
//		Vendor					Product
		"8BitDo",				"Ultimate 2 Wireless Controller",

//		Poll	StickTech		TriggerTech		JoyDead	TrgDead	AxesLat	ButnLat	AxesJit	ButnJit
//		(Hz)									(%)		(%)		(ms)	(ms)	(ms)	(ms)
		956,	tDispTech::TMR,	tDispTech::HAL,	0.05f,	0.00f,	7.02f,	2.81f,	0.45f,	0.35f		// No dead zone.
	};

	ContDict[ {0x2DC8, 0x3106} ] =
	{
		"8BitDo",				"Ultimate Bluetooth Controller",
		100,	tDispTech::HAL,	tDispTech::HAL,	0.05f,	0.00f,	16.22f,	10.11f,	2.68f,	2.56f		// No dead zone.
	};

	ContDict[ {0x045E, 0x02FF} ] =
	{
		"Microsoft",			"XBox One Controller",
		125,	tDispTech::POT,	tDispTech::POT,	0.05f,	0.00f,	5.54f,	5.54f,	2.24f,	2.24f		// No dead zone. Latencies and jitter not measures separately so they match.
	};
}
