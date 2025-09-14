// tUnitContinuousAxis.h
//
// This file implements the axis input unit. Units read single values from hardware. One or more units
// make a component.
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
#include <Foundation/tName.h>
#include <Math/tFilter.h>
#include "Input/tUnit.h"
namespace tInput
{


// An axis unit is a container for a float in [-1.0, 1.0].
class tUnitContinuousAxis : public tUnit
{
public:
	tUnitContinuousAxis(const tName& name, std::mutex& mutex)															: tUnit(name, mutex) { }
	virtual ~tUnitContinuousAxis()																						{ }

	// Thread-safe. Read by the main system update to send change notification events. May also be used directly by
	// client code in main thread.
	float GetAxis() const
	{
		std::lock_guard<std::mutex> lock(Mutex);
		return FilteredAxis.GetValue();
	}


private:

	// Called by the controller polling thread.
	void UpdateAxisRaw(float axis)
	{
		std::lock_guard<std::mutex> lock(Mutex);
		tMath::tiClamp(axis, -1.0f, 1.0f);
		FilteredAxis.Update(axis);
	}

	tMath::tLowPassFilter_FixFlt FilteredAxis;		// Mutex protected.
};


}
