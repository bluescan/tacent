// tUnitContinuousDisp.h
//
// This file implements the displacement input unit. Units read single values from hardware. One or more units
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
#include <mutex>
#include <Foundation/tFundamentals.h>
#include <Math/tFilter.h>
#include "Input/tUnit.h"
namespace tInput
{


// A continuous displacement unit is a container for a float in [0.0, 1.0].
class tUnitContinuousDisp : public tUnit
{
public:
	tUnitContinuousDisp(const tName& name, std::mutex& mutex)															: tUnit(name, mutex) { }
	virtual ~tUnitContinuousDisp()																						{ }

	// Read by the main system update to send change notification events.
	// May also be used directly by client code in main thread.
	float GetDisp() const
	{
		std::lock_guard<std::mutex> lock(Mutex);
		return FilteredDisp.GetValue();
	}

	void Reset()
	{
		//SetDisplacementRaw(0.0f);
		//SetDisplacement(0.0f);
	}

private:
	friend class tCompTrigger;

	// Called by the controller polling thread.
	void UpdateDispRaw(float disp)
	{
		std::lock_guard<std::mutex> lock(Mutex);
		tMath::tiClamp(disp, 0.0f, 1.0f);
		FilteredDisp.Update(disp);
	}

	// Called by the the controller component in the update function of the main thread. It is the component that does
	// anti-jitter and dead zone to read the raw value and convert it to the actual.
	void SetDisplacement(float disp)
	{
		tAssert(tMath::tInInterval(disp, 0.0f, 1.0f));
		std::lock_guard<std::mutex> lock(Mutex);
		//DisplacementRaw = disp;
	}

	tMath::tLowPassFilter_FixFlt FilteredDisp;		// Mutex protected.

	/// @todo Use a low pass filter here.
	//float DisplacementRaw		= 0.0f;			
	//float Displacement			= 0.0f;			// Mutel protected.
};


}
