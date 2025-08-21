// tControllerInfo.cpp
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
#include "Input/tControllerInfo.h"


namespace tInput
{
	tMap<tControllerVidPid, tControllerInfo> ControllerDictionary;

	bool ControllerDictionaryPopulated = false;
	void PopulateControllerDictionary();
}


const tInput::tControllerInfo* tInput::FindControllerInfo(const tControllerVidPid& vidpid)
{
	if (!ControllerDictionaryPopulated)
	{
		PopulateControllerDictionary();
		ControllerDictionaryPopulated = true;
	}

	return &ControllerDictionary[vidpid];
}


void tInput::PopulateControllerDictionary()
{
	ControllerDictionary[{0x1234, 0x5678}] =
	{
		"testname", 1000,
		tDisplacementTechnology::TMR, tDisplacementTechnology::TMR,
		0.1f, 0.0f, 4.0f, 3.0f, 0.5f, 0.4f
	};
	ControllerDictionary[{0x4321, 0xABCD}] =
	{
		"testname2", 125,
		tDisplacementTechnology::HALL, tDisplacementTechnology::HALL,
		0.1f, 0.0f, 4.0f, 3.0f, 0.5f, 0.4f
	};
}
