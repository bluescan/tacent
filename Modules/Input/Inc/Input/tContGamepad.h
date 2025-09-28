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


class tContGamepad : public tController
{
public:
	// Move constructor. Must be present for std::vector reserve.
	tContGamepad(tContGamepad&& src) :
		tController(src.Name),
		InitCompMove(LStick),
		InitCompMove(RStick),
		InitCompMove(DPad),
		InitCompMove(LTrigger),
		InitCompMove(RTrigger),
		InitCompMove(LViewButton),
		InitCompMove(RMenuButton),
		InitCompMove(LBumperButton),
		InitCompMove(RBumperButton),
		InitCompMove(XButton),
		InitCompMove(YButton),
		InitCompMove(AButton),
		InitCompMove(BButton),
		GamepadID(src.GamepadID)																						{ }

	// Constructs an initially disconnected (non-polling) controller. All gamepads must be contructed with an ID and a
	// unique tName.
	tContGamepad(const tName& name, tGamepadID id) :
		tController(name),
		InitCompCopy(LStick),
		InitCompCopy(RStick),
		InitCompCopy(DPad),
		InitCompCopy(LTrigger),
		InitCompCopy(RTrigger),
		InitCompCopy(LViewButton),
		InitCompCopy(RMenuButton),
		InitCompCopy(LBumperButton),
		InitCompCopy(RBumperButton),
		InitCompCopy(XButton),
		InitCompCopy(YButton),
		InitCompCopy(AButton),
		InitCompCopy(BButton),
		GamepadID(id)																									{ }
	virtual ~tContGamepad()																								{ StopPolling(); }

	tCompJoystick LStick;		// Contains the Button and 2 axes.
	tCompJoystick RStick;		// Contains the Button and 2 axes.
	tCompDirPad DPad;
	tCompTrigger LTrigger;
	tCompTrigger RTrigger;
	tCompButton LViewButton;
	tCompButton RMenuButton;
	tCompButton LBumperButton;
	tCompButton RBumperButton;
	tCompButton XButton;
	tCompButton YButton;
	tCompButton AButton;
	tCompButton BButton;

	// If pollingPeriod is <= 0 the polling period will be determined by looking it up from the controller definition
	// which is based on the vendor and product ID. If tau is < 0.0f the filter tau is determined by looking it up from
	// the controller definition which is based on the vendor and product ID. A tau of 0 is valid and results in no
	// filtering.
	void StartPolling(int pollingPeriod_us = 0, float tau_s = -1.0f);
	void StopPolling();
	bool IsPolling() const { return PollingThread.joinable(); }
	bool IsConnected() const { return IsPolling(); }
	void Update();

	// If you want to override the polling period and tau for a currently connected controller you can call this. It is
	// not something you want to call oftent because it has to stop and start the polling thread so it can reset the
	// filters. If the controller is not connected/polling this function returns false.
	bool SetPollingParameters(int pollingPeriod_us = 0, float tau_s = -1.0f);

	// For informational purposes you may want to display the current tau and or polling period. Since the period and
	// tau are atomics these functions are thread safe.
	int GetPollingPeriod() const /* In us. */ { return PollingPeriod_us; }
	float GetAxesTau() const /* In s. */ { return AxesTau_s; }
	void GetConfig(int& pollingPeriod, float& axesTau) { pollingPeriod = PollingPeriod_us; axesTau = AxesTau_s; }

private:
	void SetDefinition();
	void ClearDefinition();
	void Configure();

	// This function runs on the polling thread for this controller.
	void Poll();

	tGamepadID GamepadID = tGamepadID::Invalid;

	// The PollExitRequested predicate is required to avoid 'spurious wakeups'. Mutex protected.
	bool PollingExitRequested = false;
	std::condition_variable PollingExitCondition;

	// We consider the controller connected if the PollingThread is joinable.
	std::thread PollingThread;

	ulong PollingPacketNumber = ulong(-1);

	// This is the actual polling period being used. It will always be > 0 when polling is active. It is stored as a
	// member so the polling thread function knows how long to sleep for. It may also be retrieved for informational
	// purposes. It is always 0 when not polling and 0 is
	// considered invalid while polling.
	std::atomic<int> PollingPeriod_us = 0;

	// This is the actual tau used for the left and right joystick filters. It is always >= 0.0f when polling is active
	// and is < 0.0f (invalid) when not polling. It may be retrieved for informational purposes.
	std::atomic<float> AxesTau_s = -1.0f;
};


}
