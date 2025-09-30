// tCompJoystick.h
//
// This file implements a joystick input component. Components are input classes that are grouped together
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
#include "Math/tVector2.h"
#include "Input/tComp.h"
#include "Input/tUnitContinuousAxis.h"
#include "Input/tUnitDiscreteBool.h"
namespace tInput
{


class tCompJoystick : public tComponent
{
public:
	tCompJoystick(const tName& name, std::mutex& mutex) :
		tComponent(name),
		InitUnit(XAxis),
		InitUnit(YAxis),
		InitUnit(Button)																								{ }
	virtual ~tCompJoystick()																							{ }

	void Configure(float fixedDeltaTime, float tau, float deadZoneRadius)
	{
		XAxis.Configure(fixedDeltaTime, tau);
		YAxis.Configure(fixedDeltaTime, tau);
		DeadZoneRadius = deadZoneRadius;
	}

	// Returns false if in dead zone and true if outside (safe-zone). The x and y axis vars will have the filtered
	// result. These values are still returned inside the dead-zone but are not considered as reliable.
	bool GetAxes(tMath::tVector2& axes)
	{
		XAxis.GetAxis(axes.x);
		YAxis.GetAxis(axes.y);
		return !InDeadZone;
	}

	bool GetAxes(tMath::tVector2& axes, tMath::tVector2& rawAxes)
	{
		XAxis.GetAxis(axes.x, rawAxes.x);
		YAxis.GetAxis(axes.y, rawAxes.y);
		return !InDeadZone;
	}

	// This direction/magnitude accessor is similar to above but populates a normalized direction vector and a separate
	// length. If the axes length is zero the resultant direction is set to the zero vector and the magnitude is set to zero.
	bool GetDirectionMagnitute(tMath::tVector2& direction, float& magnitude)
	{
		GetAxes(direction);
		magnitude = direction.NormalizeSafeGetLength();
		return !InDeadZone;
	}
	
	// @todo Filtering is dealt with in polling thread. This main thread update
	// call needs to deal with the dead-zone.
	void Update()
	{
		if (DeadZoneRadius <= 0.0f)
			InDeadZone = false;

		// I think it makes slightly more sense to consider the joystick in the deadzone if the raw values indicate it
		// is rather than the filtered values. The GetAxis calls use a mutex to protect the polled values.
		float x, y, rx, ry;
		XAxis.GetAxis(x, rx); YAxis.GetAxis(y, ry);
		tMath::tVector2 rv(rx, ry);
		InDeadZone = (rv.LengthSq() <= DeadZoneRadius*DeadZoneRadius);
	}

private:
	friend class tContGamepad;

	void SetAxesRaw(float xaxis, float yaxis)
	{
		XAxis.SetAxisRaw(xaxis);
		YAxis.SetAxisRaw(yaxis);
	}

	// These are private because they need to be mutex-protected. Use the accessors.
	tUnitContinuousAxis XAxis;		// Horizontal.
	tUnitContinuousAxis YAxis;		// Vertical.

	float DeadZoneRadius = 0.0f;
	bool InDeadZone = false;

	// Pressing down on the stick. By having this button in the joystick component we can, if we want, deal with the
	// fact that there is mechanical linkage between the button and the axes. There is likely more unwanted movement in
	// the axes after the button is pressed. Of course before the actual click-down there will be extra movement also,
	// but we have no way to detect that.
	tUnitDiscreteBool Button;
};


}
