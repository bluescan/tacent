// tContGamepad.h
//
// This file implements a gamepad controller. Controllers represent physical input devices.
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
#include "Input/tCont.h"
#include "Input/tCompJoystick.h"		// A gamepad has 2 joysticks. Joysticks contain the push button.
#include "Input/tCompDirPad.h"			// A gamepad has 1 DPad.
#include "Input/tCompTrigger.h"			// A gamepad has 2 Triggers.
#include "Input/tCompButton.h"			// A gamepad has 8 Buttons.
namespace tInput
{


class tContGamepad : public tController
{
public:
	tContGamepad(std::mutex& mutex)																						: tController(), Mutex(mutex) { }
	virtual ~tContGamepad()																								{ }

	tCompJoystick LStick;		// Contains the Button and 2 axes.
	tCompJoystick RStick;		// Contains the Button and 2 axes.
	tCompDirPad DPad;
	tCompTrigger LTrigger;
	tCompTrigger RTrigger;
	tCompButton View;			// On Left
	tCompButton Menu;			// On Right.
	tCompButton LBumper;
	tCompButton RBumper;
	tCompButton X;
	tCompButton Y;
	tCompButton A;
	tCompButton B;

	bool IsConnected() const
	{
		const std::lock_guard<std::mutex> lock(Mutex);
		return Connected;
	}
	void SetConnected(int connected)
	{
		const std::lock_guard<std::mutex> lock(Mutex);
		Connected = connected;
	}

private:
	std::mutex& Mutex;

	// Connectedness is mutex protected.
	bool Connected;
};


}
