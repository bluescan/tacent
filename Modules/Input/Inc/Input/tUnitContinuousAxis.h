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

	// Alternate versions of GetAxis that fill in value references. These functions lock a mutex it's best to only do
	// one function call to get all the values you need.
	void GetAxis(float& filteredAxis)
	{
		std::lock_guard<std::mutex> lock(Mutex);
		filteredAxis = FilteredAxis.GetValue();
	}

	void GetAxis(float& filteredAxis, float& rawAxis)
	{
		std::lock_guard<std::mutex> lock(Mutex);
		filteredAxis = FilteredAxis.GetValue();
		rawAxis = RawAxis;
	}

private:
	friend class tCompJoystick;

	// This is called by the controller before polling starts.
	void Configure(float fixedDeltaTime, float filterTau)
	{
		FilteredAxis.Set(fixedDeltaTime, filterTau, true);
	}

	// Called by the controller in the polling thread.
	void UpdateAxisRaw(float rawAxis)
	{
		std::lock_guard<std::mutex> lock(Mutex);
		tMath::tiClamp(rawAxis, -1.0f, 1.0f);
		FilteredAxis.Update(rawAxis);

		RawAxis = rawAxis;
	}

	// Called by the controller in the polling thread. This is called if there was no change in value... but we still
	// need an update so the fixed period filter gets updated.
	void UpdateAxisSame()
	{
		std::lock_guard<std::mutex> lock(Mutex);
		FilteredAxis.Update(RawAxis);
	}

	float RawAxis									= 0.0f;
	tMath::tLowPassFilter_FixFlt FilteredAxis;		// Mutex protected.
};


}
