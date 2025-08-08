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
#include <condition_variable>
#include "Input/tCont.h"
#include "Input/tCompJoystick.h"		// A gamepad has 2 joysticks. Joysticks contain the push button.
#include "Input/tCompDirPad.h"			// A gamepad has 1 DPad.
#include "Input/tCompTrigger.h"			// A gamepad has 2 Triggers.
#include "Input/tCompButton.h"			// A gamepad has 8 Buttons (not including the joystick buttons).
namespace tInput
{


enum class tGamepadID
{
	Invalid = -1,
	GP0, GP1, GP2, GP3,
	NumGamepads
};


#define InitCompMove(n) n(src.Name+"|##n")
#define InitCompMovM(n) n(src.Name+"|##n", Mutex)

class tContGamepad : public tController
{
public:
	// Move constructor. Must be present for std::vector reserve.
	tContGamepad(tContGamepad&& src) :
		tController(src.Name),
		InitCompMove(LStick),
		InitCompMove(RStick),
		InitCompMove(DPad),
		InitCompMovM(LTrigger),

		/// WIP
		RTrigger(src.Name+"|RTrigger", Mutex),

		View(src.Name+"|View"),
		Menu(src.Name+"|Menu"),
		LBumper(src.Name+"|LBumper"),
		RBumper(src.Name+"|RBumper"),
		X(src.Name+"|X"),
		Y(src.Name+"|Y"),
		A(src.Name+"|A"),
		B(src.Name+"|B"),
		GamepadID()
	{		
	}

	// Constructs an initially disconnected (non-polling) controller. All gamepads must be contructed with an ID and a
	// unique tName.
	tContGamepad(const tName& name, tGamepadID id) :
		tController(name),
		LStick(name+"|LStick"),
		RStick(name+"|RStick"),
		DPad(name+"|DPad"),
		LTrigger(name+"|LTrigger", Mutex),
		RTrigger(name+"|RTrigger", Mutex),
		View(name+"|View"),
		Menu(name+"|Menu"),
		LBumper(name+"|LBumper"),
		RBumper(name+"|RBumper"),
		X(name+"|X"),
		Y(name+"|Y"),
		A(name+"|A"),
		B(name+"|B"),
		GamepadID(id)
	{
	}
	virtual ~tContGamepad()																								{ StopPolling(); }

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

	void StartPolling(int pollingPeriod, tGamepadID);
	void StopPolling();
	bool IsPolling() const { return PollingThread.joinable(); }
	bool IsConnected() const { return IsPolling(); }
	void Update();

private:

	// This function runs on the polling thread for this controller.
	void Poll();

	tGamepadID GamepadID = tGamepadID::Invalid;

	// Protects updates to all the components since they may be read by the main thread at any time.
	// Protects PollExitRequested.
	std::mutex Mutex;

	// The PollExitRequested predicate is required to avoid 'spurious wakeups'. Mutex protected.
	bool PollingExitRequested = false;
	std::condition_variable PollingExitCondition;

	// We consider the controller connected if the PollingThread is joinable.
	std::thread PollingThread;

	ulong PollingPacketNumber = ulong(-1);
	tGamepadID PollingGamepadID = tGamepadID::Invalid;
	int PollingPeriod = 0;
};


}
