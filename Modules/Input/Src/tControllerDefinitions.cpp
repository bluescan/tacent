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
	// The controller dictionary singleton. Lookup by vidpid and contains the controller definitions.
	struct tControllerDict { static tMap<tVidPid, tControllerDefinition>& Get()
	{
		static tMap<tVidPid, tControllerDefinition> ControllerDictionary;
		return ControllerDictionary;
	} };

	bool ControllerDictionaryPopulated = false;
}


void tInput::tInitializeControllerDictionary()
{
	tControllerDict::Get()[ {0x2DC8, 0x310B} ] =
	{
		//Vendor				Product
		"8BitDo",				"Ultimate 2 Wireless Controller",

		//Poll	StickTech		TriggerTech		StkDead	TrgDead	AxesLat	ButnLat	AxesJit	ButnJit
		//(Hz)									(%)		(%)		(ms)	(ms)	(ms)	(ms)
		1000,	tDispTech::TMR,	tDispTech::HAL,	0.05f,	0.00f,	7.00f,	2.80f,	0.45f,	0.35f		// No dead zone.
	};

	tControllerDict::Get()[ {0x2DC8, 0x3106} ] =
	{
		"8BitDo",				"Ultimate Bluetooth Controller",
		100,	tDispTech::HAL,	tDispTech::HAL,	0.05f,	0.00f,	16.20f,	10.10f,	2.70f,	2.60f		// No dead zone.
	};

	tControllerDict::Get()[ {0x045E, 0x02FF} ] =
	{
		"Microsoft",			"XBox One Controller",
		125,	tDispTech::POT,	tDispTech::POT,	0.05f,	0.00f,	5.50f,	5.50f,	2.20f,	2.20f		// No dead zone. Latencies and jitter not measures separately so they match.
	};

	ControllerDictionaryPopulated = true;
}


void tInput::tShutdownControllerDictionary()
{
	ControllerDictionaryPopulated = false;
	tControllerDict::Get().Clear();
}


const tInput::tControllerDefinition* tInput::tLookupControllerDefinition(const tVidPid& vidpid)
{
	tAssert(ControllerDictionaryPopulated);
	return tControllerDict::Get().GetValue(vidpid);
}
