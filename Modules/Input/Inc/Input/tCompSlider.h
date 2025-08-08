// tCompSlider.h
//
// This file implements a slider input component. Components are input classes that are grouped together
// in a device.
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

#pragma once
#include <mutex>
#include <Foundation/tName.h>
#include "Input/tComp.h"
#include "Input/tUnitContinuousDisp.h"
namespace tInput
{


class tCompSlider : public tComponent
{
public:
	tCompSlider(const tName& name, std::mutex& mutex)																	: tComponent(name), Displacement(mutex) { }
	virtual ~tCompSlider()																								{ }

private:
	// These are private because they need to be mutex-protected. Use the accessors.
	tUnitContinuousDisp Displacement;
};


}
