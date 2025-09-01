// tContGamepad.cpp
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

#include <chrono>
#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <xinput.h>
#endif
#include "Foundation/tPlatform.h"
#include "Input/tContGamepad.h"
#include "System/tPrint.h"
namespace tInput
{


void tContGamepad::StartPolling(int pollingPeriod_ns)
{
	// If it's already running do nothing. We also don't update the period if we're already running.
	if (IsPolling())
		return;

	// Set the controller definition.
	SetDefinition();

	// PollingPeriod is only ever set before the thread starts and doesn't change. We don't need to Mutex protect it.
	if (pollingPeriod_ns <= 0)
		PollingPeriod_ns = int(1000000.0f/Definition.MaxPollingFreq);
	else
		PollingPeriod_ns = pollingPeriod_ns;

	PollingThread = std::thread(&tContGamepad::Poll, this);
}


void tContGamepad::StopPolling()
{
	// If it's already stopped so do nothing.
	if (!IsPolling())
		return;

	// We don't want to wait around to exit the polling thread to finish its sleep cycle so we use a condition_variable
	// that uses PollingExitRequested as the predecate.
	{
		const std::lock_guard<std::mutex> lock(Mutex);
		PollingExitRequested = true;
	}

	// Joins back up to the detection thread.
	PollingThread.join();
	ClearDefinition();
}


void tContGamepad::Poll()
{
	while (true)
	{
		#ifdef PLATFORM_WINDOWS
		WinXInputState state;
		tStd::tMemclr(&state, sizeof(WinXInputState));

		// Get the state of the controller from XInput. From what I can tell reading different gamepads on different
		// threads does not require a mutex to protect this call. Only this thread instance will read this particular
		// controller and so two calls to XInputGetState for the same controller will never happen at the same time.
		WinDWord result = XInputGetState(int(GamepadID), &state);

		if (result == WinErrorSuccess)
		{
			// Controller connected. We can read its state and update components.
			if (state.dwPacketNumber != PollingPacketNumber)
			{
				// The set calls to the components below are all mutex protected internally. Main thread may read input
				// unit values so we require the protection.

				// Note there is a bit more precision for the neg integral values. -32768 to 32767 maps to [-1.0, 1.0]
				int16 rawLX = state.Gamepad.sThumbLX;
				float axisLXNorm = (rawLX < 0) ? float(rawLX)/32768.0f : float(rawLX)/32767.0f;
				tPrintf("LX: %05.3f ", axisLXNorm);
				// @todo Set LStick X.

				int16 rawLY = state.Gamepad.sThumbLY;
				float axisLYNorm = (rawLY < 0) ? float(rawLY)/32768.0f : float(rawLY)/32767.0f;
				tPrintf("LY: %05.3f ", axisLXNorm);
				// @todo Set LStick Y.

				bool rawLB = (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) ? true : false;
				tPrintf("LB: %s\n", rawLB ? "down" : "up");
				// @todo Set LStick B.

				int16 rawRX = state.Gamepad.sThumbRX;
				float axisRXNorm = (rawRX < 0) ? float(rawRX)/32768.0f : float(rawRX)/32767.0f;
				// @todo Set RStick X.

				int16 rawRY = state.Gamepad.sThumbRY;
				float axisRYNorm = (rawRY < 0) ? float(rawRY)/32768.0f : float(rawRY)/32767.0f;
				// @todo Set RStick Y.

				bool rawRB = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) ? true : false;

				float leftTriggerNorm = float(state.Gamepad.bLeftTrigger) / 255.0f;
				LTrigger.SetDisplacementRaw(leftTriggerNorm);

				float rightTriggerNorm = float(state.Gamepad.bRightTrigger) / 255.0f;
				RTrigger.SetDisplacementRaw(rightTriggerNorm);

				PollingPacketNumber = state.dwPacketNumber;
			}
		}
		else
		{
			// Controller is not connected. This can happen if the detection thread of the controller system has not
			// realized yet that the controller was disconnected (and stopped the polling thread). In this case we can
			// just stop polling. Note that exiting the loop does not stop the thread from being joinable. It is still
			// considered connected until the detection thread calls StopPolling. In the mean time, it only makes sense
			// to reset the components.
			LTrigger.Reset();
			RTrigger.Reset();
			break;
		}
		#endif

		// This unique_lock is just a more powerful version of lock_guard. Supports subsequent unlocking/locking which
		// is presumably needed by wait_for. In any case, wait_for needs this type of lock.
		std::unique_lock<std::mutex> lock(Mutex);
		bool exitRequested = PollingExitCondition.wait_for(lock, std::chrono::nanoseconds(PollingPeriod_ns), [this]{ return PollingExitRequested; });
		if (exitRequested)
			break;
	}
}


void tContGamepad::Update()
{
	LStick.Update();
	RStick.Update();
	DPad.Update();
	LTrigger.Update();
	RTrigger.Update();
	LViewButton.Update();
	RMenuButton.Update();
	LBumperButton.Update();
	RBumperButton.Update();
	XButton.Update();
	YButton.Update();
	AButton.Update();
	BButton.Update();
}


void tContGamepad::SetDefinition()
{
	#ifdef PLATFORM_WINDOWS

	tPrintf("Gamepad %d Set Definition", int(GamepadID));
	WinXInputCapabilitiesEx capsEx;
	tStd::tMemclr(&capsEx, sizeof(WinXInputCapabilitiesEx));
	if (XInputGetCapabilitiesEx(1, int(GamepadID), 0, &capsEx) == WinErrorSuccess)
	{
		tVidPid vidpid(uint16(capsEx.vendorId), uint16(capsEx.productId));
		const tContDefn* defn = tLookupContDefn(vidpid);
		const char* vendor = defn ? defn->Vendor : "unknown";
		const char* product = defn ? defn->Product : "unknown";
		tPrintf
		(
			"Gamepad vid = 0x%04X pid = 0x%04X Vendor:%s Product:%s\n",
			int(capsEx.vendorId), int(capsEx.productId), vendor, product
		);
		if (defn)
			Definition = *defn;
		else
			Definition.SetGeneric();
	}
	else
	{
		tPrintf("Using generic definition\n");
		Definition.SetGeneric();
	}

	#else
	Definition.SetGeneric();
	#endif
}


void tContGamepad::ClearDefinition()
{
	Definition.Clear();
}


}
